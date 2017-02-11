/*
 * \defgroup RECEIVE Receive Functions
 *
 * \brief These functions handle receiving radio packets and acting on them.
 *
 * For camera packets, each received packet is decoded, and the most recent data is made available for the tick functions elsewhere to act on.
 * Also, a camera packet that requests feedback immediately (i.e. a status update request) notifies the feedback module.
 *
 * For message packets, the action required by the packet is taken immediately.
 *
 * \{
 */

#include "receive.h"
#include "charger.h"
#include "chicker.h"
#include "dma.h"
#include "dr.h"            
#include "feedback.h"
#include "leds.h"
#include "main.h"
#include "motor.h"
#include "mrf.h"
#include "priority.h"
#include "rtc.h"
#include "primitives/primitive.h"
#include "physics.h"
#include <FreeRTOS.h>
#include <assert.h>
#include <semphr.h>
#include <stack.h>
#include <task.h>
#include <unused.h>

/**
 * \brief The number of robots in the drive packet.
 */
#define RECEIVE_DRIVE_NUM_ROBOTS 8

/**
 * \brief The number of bytes of drive packet data per robot.
 */
#define RECEIVE_DRIVE_BYTES_PER_ROBOT 9

/**
 * \brief The number of bytes of camera data per robot.
 */
#define CAMERA_BYTES_PER_ROBOT 6

/**
 * \brief The minimum number of bytes in a camera frame.
 */
#define CAMERA_MIN_BYTES 1 /*Mask*/ + 1 /*Flag*/ + 0 /*Ball Data*/ + 0 /*Robot Data*/ + 8 /*Timestamp*/ + 1 /*Status*/

static unsigned int robot_index;
static uint8_t *dma_buffer;
static SemaphoreHandle_t drive_mtx;
static unsigned int timeout_ticks;
static uint8_t last_serial = 0xFF;

static void receive_task(void *UNUSED(param)) {

  uint16_t last_sequence_number = 0xFFFFU;
  bool last_estop_run = false;
  bool estop_run = false;
  size_t frame_length;
  uint32_t i;

  while ((frame_length = mrf_receive(dma_buffer))) {


    uint16_t frame_control = dma_buffer[0U] | (dma_buffer[1U] << 8U);
    // Sanity-check the frame control word
    if (((frame_control >> 0U) & 7U) == 1U /* Data packet */ && ((frame_control >> 3U) & 1U) == 0U /* No security */ && ((frame_control >> 6U) & 1U) == 1U /* Intra-PAN */ && ((frame_control >> 10U) & 3U) == 2U /* 16-bit destination address */ && ((frame_control >> 14U) & 3U) == 2U /* 16-bit source address */) {
      // Read out and check the source address and sequence number
      uint16_t source_address = dma_buffer[7U] | (dma_buffer[8U] << 8U);
      uint8_t sequence_number = dma_buffer[2U];
      if (source_address == 0x0100U && sequence_number != last_sequence_number) {

        // Update sequence number
        last_sequence_number = sequence_number;

        // Handle packet
        uint16_t dest_address = dma_buffer[5U] | (dma_buffer[6U] << 8U);
	static const size_t HEADER_LENGTH = 2U /* Frame control */ + 1U /* Seq# */ + 2U /* Dest PAN */ + 2U /* Dest */ + 2U /* Src */;
	static const size_t FOOTER_LENGTH = 2U /* FCS */ + 1U /* RSSI */ + 1U /* LQI */;
	if (dest_address == 0xFFFFU) {
	  // Broadcast frame must contain a camera packet.
          // Note that camera packets have a variable length.          
          uint32_t buffer_position = HEADER_LENGTH;
          
          // The first step is to get the mask vector, which contains
          // which robots have valid camera data.
          uint8_t mask_vector = dma_buffer[buffer_position++];
          
          // The next byte contains the flag information.
          uint8_t flag_vector = dma_buffer[buffer_position++];
          bool contains_ball = !!(flag_vector & 0x04);
          bool contains_timestamp = !!(flag_vector & 0x02);
          last_estop_run = estop_run;
          estop_run = !(flag_vector & 0x01); 
          bool contains_robot = false;

          // If the contains_ball flag was set, the next four bytes
          // contain the ball position.
          if (contains_ball) {
            int16_t ball_x = 0;
            int16_t ball_y = 0;

            ball_x |= dma_buffer[buffer_position++];
            ball_x |= (dma_buffer[buffer_position++] << 8);
            ball_y |= dma_buffer[buffer_position++];
            ball_y |= (dma_buffer[buffer_position++] << 8);
          
            dr_set_ball_frame(ball_x, ball_y);
          }
          
          // The next bytes contain the robot camera information, if any.
          for (i = 0; i < RECEIVE_DRIVE_NUM_ROBOTS; i++) {
            if ((0x01 << i) & mask_vector) {
              // Valid camera data for robot i, if i matches this robot's 
              // index, update camera data.
              if (i == robot_index) {
            	timeout_ticks = 1000U / portTICK_PERIOD_MS;

                int16_t robot_x = 0;
                int16_t robot_y = 0;
                int16_t robot_angle = 0;
                contains_robot = true;
                     
                robot_x |= dma_buffer[buffer_position++];
                robot_x |= (dma_buffer[buffer_position++] << 8);
                robot_y |= dma_buffer[buffer_position++];
                robot_y |= (dma_buffer[buffer_position++] << 8);
                robot_angle |= dma_buffer[buffer_position++];
                robot_angle |= (dma_buffer[buffer_position++] << 8);
              }
              else {
                buffer_position += 6;
              }
            }
          }

          // If the packet contains a timestamp, it will be in the next 
          // 8 bytes.
          if (contains_timestamp) {
            uint64_t timestamp = 0;
            for (i = 0; i < 8; i++) {
               timestamp |= ((uint64_t)dma_buffer[buffer_position++] << 8*i);
            }
            rtc_set(timestamp);

            // If this packet contained robot or ball information, update
            // the timestamp for the camera data.
            if (contains_robot) {
              dr_set_robot_timestamp(timestamp);
            }
            if (contains_ball) {
              dr_set_ball_timestamp(timestamp);
            }
          }
            
          // The final byte is the status byte.
          uint8_t status = dma_buffer[buffer_position++];
          if ((status & 0x07) == robot_index) {
            feedback_pend_normal();   
          }
         
          // If the estop has been switched off, execute the stop primitive. 
          if (!estop_run) {
	    primitive_params_t pparams;
            for (i = 0; i < 4; i++) {
               pparams.params[i] = 0;
            }
            pparams.slow = false;
            pparams.extra = 0;
	    // Take the drive mutex.
	    xSemaphoreTake(drive_mtx, portMAX_DELAY);
	    // Reset timeout.
	    timeout_ticks = 1000U / portTICK_PERIOD_MS;
            primitive_start(0, &pparams);
            // Return the drive mutex.
	    xSemaphoreGive(drive_mtx);
          } else{
	    xSemaphoreTake(drive_mtx, portMAX_DELAY);
	    //dr_do_maneuver();
	    dr_follow_ball();
	    xSemaphoreGive(drive_mtx);
	   }        
        } 
        
        // Otherwise, it is a message packet specific to this robot.
        else if (frame_length >= HEADER_LENGTH + 1U + FOOTER_LENGTH) {
          static const uint16_t MESSAGE_PURPOSE_ADDR = HEADER_LENGTH;
	  static const uint16_t MESSAGE_PAYLOAD_ADDR = MESSAGE_PURPOSE_ADDR + 1U;
	  primitive_params_t pparams;
          switch (dma_buffer[MESSAGE_PURPOSE_ADDR]) {
            case 0x00:
	      if (frame_length == HEADER_LENGTH + 4U + FOOTER_LENGTH) {
	        uint8_t which = dma_buffer[MESSAGE_PAYLOAD_ADDR];
		uint16_t width = dma_buffer[MESSAGE_PAYLOAD_ADDR + 2U];
		width <<= 8U;
		width |= dma_buffer[MESSAGE_PAYLOAD_ADDR + 1U];
		chicker_fire(which ? CHICKER_CHIP : CHICKER_KICK, width);
	      }
	      break;
	    case 0x01U: // Arm autokick
	      if (frame_length == HEADER_LENGTH + 4U + FOOTER_LENGTH) {
	        uint8_t which = dma_buffer[MESSAGE_PAYLOAD_ADDR];
		uint16_t width = dma_buffer[MESSAGE_PAYLOAD_ADDR + 2U];
		width <<= 8U;
		width |= dma_buffer[MESSAGE_PAYLOAD_ADDR + 1U];
		chicker_auto_arm(which ? CHICKER_CHIP : CHICKER_KICK, width);
	      }
	      break;

	    case 0x02U: // Disarm autokick
	      if (frame_length == HEADER_LENGTH + 1U + FOOTER_LENGTH) {
	        chicker_auto_disarm();
	      }
	      break;

	    case 0x03U: // Set LED mode
	      if (frame_length == HEADER_LENGTH + 2U + FOOTER_LENGTH) {
	        uint8_t mode = dma_buffer[MESSAGE_PAYLOAD_ADDR];
		if (mode <= 4U) {
		  leds_test_set_mode(LEDS_TEST_MODE_HALL, mode);
		}else if (5U <= mode && mode <= 8U) {
		  leds_test_set_mode(LEDS_TEST_MODE_ENCODER, mode - 5U);
		}else if (mode == 0x21U) {
		  leds_test_set_mode(LEDS_TEST_MODE_CONSTANT, 0x7U);
		}else {
		  leds_test_set_mode(LEDS_TEST_MODE_NORMAL, 0U);
		}
	      }
	      break;
	    case 0x08U: // Reboot
	      if (frame_length == HEADER_LENGTH + 1U + FOOTER_LENGTH) {
	        main_shutdown(MAIN_SHUT_MODE_REBOOT);
	      }
	      break;
	    case 0x09U: // Force on motor power
	      if (frame_length == HEADER_LENGTH + 1U + FOOTER_LENGTH) {
	        motor_force_power();
	      }
	      break;
 
	    case 0x0CU: // Shut down
	      if (frame_length == HEADER_LENGTH + 1U + FOOTER_LENGTH) {
	        main_shutdown(MAIN_SHUT_MODE_POWER);
	      }
	      break;

	    case 0x0DU: // Request build IDs
	      feedback_pend_build_ids();
	      break;

            case 0x0EU: // Set capacitor bits.
              xSemaphoreTake(drive_mtx, portMAX_DELAY);
              char capacitor_flag = dma_buffer[MESSAGE_PAYLOAD_ADDR];
              charger_enable(capacitor_flag & 0x02);
              chicker_discharge(capacitor_flag & 0x01);
              xSemaphoreGive(drive_mtx);
              break;

	    primitive_params_t prim_params;
            case 0x0FU: // Stop Primitive
		xSemaphoreTake(drive_mtx, portMAX_DELAY);
		primitive_start(0, &prim_params);
		xSemaphoreGive(drive_mtx);
            case 0x10U: // Move Primitive
		xSemaphoreTake(drive_mtx, portMAX_DELAY);
		primitive_start(1, &prim_params);
		xSemaphoreGive(drive_mtx);
            case 0x11U: // Dribble Primitive
            case 0x12U: // Shoot Primitive
            case 0x13U: // Catch Primitive
            case 0x14U: // Pivot Primitive
            case 0x15U: // Spin Primitive
            case 0x16U: // Direct Wheels Primitive
            case 0x17U: // Direct Velocity Primitive
              if (estop_run) {
                // Take the drive mutex.
                xSemaphoreTake(drive_mtx, portMAX_DELAY);
                // Reset timeout.
                timeout_ticks = 1000U / portTICK_PERIOD_MS;
                for (i = 0; i < 4; i++) {
                  pparams.params[i] = dma_buffer[MESSAGE_PAYLOAD_ADDR+ 2*i + 1] << 8;
                  pparams.params[i] |= dma_buffer[MESSAGE_PAYLOAD_ADDR + 2*i];
                }
                pparams.slow = !!(dma_buffer[MESSAGE_PAYLOAD_ADDR + 9] & 0x01);
                pparams.extra = dma_buffer[MESSAGE_PAYLOAD_ADDR + 8];
                //primitive_start(dma_buffer[MESSAGE_PURPOSE_ADDR] - 0x0F, &pparams);
                // Return the drive mutex.
                xSemaphoreGive(drive_mtx);
              }
            default:
              break; 
          }
        }
      }
    }
  }
  // mrf_receive returned zero, which means a cancellation has been requested.
  // This means we are shutting down.
  xSemaphoreGive(main_shutdown_sem);
  vTaskSuspend(0);
}

/**
 * \brief Initializes the receive task.
 *
 * \param[in] index the robot index
 */
void receive_init(unsigned int index) {
	static StaticSemaphore_t drive_mtx_storage;
	drive_mtx = xSemaphoreCreateMutexStatic(&drive_mtx_storage);

	robot_index = index;

	dma_memory_handle_t dma_buffer_handle = dma_alloc(128U);
	assert(dma_buffer_handle);
	dma_buffer = dma_get_buffer(dma_buffer_handle);

	static StaticTask_t receive_task_tcb;
	STACK_ALLOCATE(receive_task_stack, 4096);
	xTaskCreateStatic(&receive_task, "rx", sizeof(receive_task_stack) / sizeof(*receive_task_stack), 0, PRIO_TASK_RX, receive_task_stack, &receive_task_tcb);
}

/**
 * \brief Stops the receive task.
 */
void receive_shutdown(void) {
	mrf_receive_cancel();
	xSemaphoreTake(main_shutdown_sem, portMAX_DELAY);
}

/**
 * \brief Ticks the receiver.
 */
void receive_tick(log_record_t *record) {
	// Decrement timeout tick counter if nonzero.      
	if (timeout_ticks == 1) {
		timeout_ticks = 0;
		charger_enable(false);
		chicker_discharge(true);

		primitive_params_t stop_params;
		xSemaphoreTake(drive_mtx, portMAX_DELAY);
		primitive_start(0, &stop_params);
		xSemaphoreGive(drive_mtx);
	} else if (timeout_ticks > 1) {
		--timeout_ticks;
	}
}


/**
 * \brief Returns the serial number of the most recent drive packet.
 */
uint8_t receive_last_serial(void) {
	return __atomic_load_n(&last_serial, __ATOMIC_RELAXED);
}



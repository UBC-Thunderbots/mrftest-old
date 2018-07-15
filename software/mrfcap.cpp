#include <glibmm/main.h>
#include <sys/signalfd.h>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <string>
#include <vector>
#include "main.h"
#include "mrf/constants.h"
#include "util/exception.h"
#include "util/fd.h"
#include "util/libusb.h"
#include "util/main_loop.h"
#include "util/string.h"

namespace
{
unsigned int packets_captured = 0;
uint64_t base_timestamp;

unsigned long long convert_number(const std::string &str)
{
    std::size_t end_pos    = 0;
    unsigned long long ull = std::stoull(str, &end_pos, 0);
    if (end_pos != str.size())
    {
        throw std::runtime_error(
            "Whole string not consumed in number conversion");
    }
    return ull;
}

uint64_t get_best_system_timestamp()
{
    auto stamp            = std::chrono::system_clock::now();
    std::time_t converted = std::chrono::system_clock::to_time_t(stamp);
    std::chrono::duration<uint64_t, std::micro> micros =
        std::chrono::duration_cast<std::chrono::duration<uint64_t, std::micro>>(
            stamp.time_since_epoch());
    if (static_cast<uint64_t>(converted) == micros.count() / UINT64_C(1000000))
    {
        // std::chrono::system_clock and time_t use the same units on this
        // platform, so we can get a fractional part.
        return micros.count();
    }
    else
    {
        // std::chrono::system_clock and time_t use different units on this
        // platform, so we can only achieve second resolution.
        return static_cast<uint64_t>(converted);
    }
}

void handle_transfer_done(AsyncOperation<void> &asyncOp, std::ostream &ofs)
{
    USB::BulkInTransfer &transfer = static_cast<USB::BulkInTransfer &>(asyncOp);
    transfer.result();
    if (transfer.size() >= 10)
    {
        if (transfer.data()[0] & 0x01)
        {
            std::cout << "Warning, dropped packet!\n";
        }
        uint32_t seconds, microseconds;
        {
            uint64_t stamp = 0;
            for (unsigned int i = 5; i <= 5; --i)
            {
                stamp <<= 8;
                stamp |= transfer.data()[2 + i];
            }
            stamp += base_timestamp;
            seconds      = static_cast<uint32_t>(stamp / UINT64_C(1000000));
            microseconds = static_cast<uint32_t>(stamp % UINT64_C(1000000));
        }

        {
            uint32_t u32;
            u32 = seconds;
            ofs.write(reinterpret_cast<const char *>(&u32), 4);
            u32 = microseconds;
            ofs.write(reinterpret_cast<const char *>(&u32), 4);
            u32 = static_cast<uint32_t>(transfer.size() - 10);
            ofs.write(reinterpret_cast<const char *>(&u32), 4);
            ofs.write(reinterpret_cast<const char *>(&u32), 4);
        }
        ofs.write(
            reinterpret_cast<const char *>(transfer.data() + 8),
            static_cast<std::streamsize>(transfer.size() - 10));
        ofs.flush();
        std::cout << "Captured packet of length " << transfer.size() << '\n';
        ++packets_captured;
    }
    else
    {
        std::cout << "Bad capture size " << transfer.size()
                  << " (must be ≥10):";
        for (std::size_t i = 0U; i < transfer.size(); ++i)
        {
            std::cout << ' ' << static_cast<unsigned int>(transfer.data()[i]);
        }
        std::cout << '\n';
    }
    transfer.submit();
}

bool handle_sigint(Glib::IOCondition)
{
    MainLoop::quit();
    return false;
}
}

int app_main(int argc, char **argv)
{
    // Set the current locale from environment variables
    std::locale::global(std::locale(""));

    // Parse command-line arguments
    if (argc != 7)
    {
        std::cerr << "Usage:\n";
        std::cerr << argv[0] << " <channel> <symbol-rate> <pan-id> "
                                "<mac-address> <capture-flags> "
                                "<capture-file>\n";
        std::cerr << '\n';
        std::cerr << "  <channel> is the channel on which to capture, a number "
                     "from 0x0B to 0x1A\n";
        std::cerr << "  <symbol-rate> is the bit rate at which to capture, "
                     "either 250 or 625\n";
        std::cerr << "  <pan-id> is the PAN ID to capture (if PAN ID filtering "
                     "is enabled), a number from 0x0001 to 0xFFFE\n";
        std::cerr << "  <mac-address> is the station’s local MAC address, a "
                     "64-bit number excluding 0, 0xFFFF, and "
                     "0xFFFFFFFFFFFFFFFF\n";
        std::cerr << "  <capture-flags> is a numerical combination of capture "
                     "flags described at "
                     "<http://trac.thecube.ca/trac/thunderbots/wiki/Electrical/"
                     "RadioProtocol/2013/USB#SetPromiscuousFlags>\n";
        std::cerr << "  <capture-file> is the name of the .pcap file to write "
                     "the captured packets into\n";
        return 1;
    }
    uint8_t channel = static_cast<uint8_t>(convert_number(argv[1]));
    unsigned int symbol_rate =
        static_cast<unsigned int>(convert_number(argv[2]));
    if (symbol_rate != 250 && symbol_rate != 625)
    {
        std::cerr << "Invalid symbol rate; must be one of 250 or 625\n";
        return 1;
    }
    uint8_t symbol_rate_encoded = symbol_rate == 625 ? 1 : 0;
    uint16_t pan_id        = static_cast<uint16_t>(convert_number(argv[3]));
    uint64_t mac_address   = static_cast<uint64_t>(convert_number(argv[4]));
    uint16_t capture_flags = static_cast<uint16_t>(convert_number(argv[5]));

    // Start listening for CTRL+C (SIGINT).
    FileDescriptor signalfd;
    {
        sigset_t sigs;
        sigemptyset(&sigs);
        sigaddset(&sigs, SIGINT);
        if (sigprocmask(SIG_BLOCK, &sigs, nullptr) < 0)
        {
            throw SystemError("sigprocmask", errno);
        }
        int rawfd = ::signalfd(-1, &sigs, SFD_NONBLOCK);
        signalfd  = FileDescriptor::create_from_fd(rawfd);
    }
    Glib::signal_io().connect(&handle_sigint, signalfd.fd(), Glib::IO_IN);

    // Open the dongle
    std::cout << "Addressing dongle… ";
    std::cout.flush();
    USB::Context ctx;
    USB::DeviceHandle devh(
        ctx, MRF::VENDOR_ID, MRF::PRODUCT_ID, std::getenv("MRF_SERIAL"));

    // Sanity-check the dongle by looking for an interface with the appropriate
    // subclass and alternate settings with the appropriate protocols.
    // While doing so, discover which interface number is used for the radio and
    // which alternate settings are for configuration-setting and normal
    // operation.
    int radio_interface = -1, configuration_altsetting = -1,
        promiscuous_altsetting = -1;
    {
        const libusb_config_descriptor &desc =
            devh.configuration_descriptor_by_value(1);
        for (int i = 0; i < desc.bNumInterfaces; ++i)
        {
            const libusb_interface &intf = desc.interface[i];
            if (intf.num_altsetting &&
                intf.altsetting[0].bInterfaceClass == 0xFF &&
                intf.altsetting[1].bInterfaceSubClass == MRF::SUBCLASS)
            {
                radio_interface = i;
                for (int j = 0; j < intf.num_altsetting; ++j)
                {
                    const libusb_interface_descriptor &as = intf.altsetting[j];
                    if (as.bInterfaceClass == 0xFF &&
                        as.bInterfaceSubClass == MRF::SUBCLASS)
                    {
                        if (as.bInterfaceProtocol == MRF::PROTOCOL_OFF)
                        {
                            configuration_altsetting = j;
                        }
                        else if (
                            as.bInterfaceProtocol == MRF::PROTOCOL_PROMISCUOUS)
                        {
                            promiscuous_altsetting = j;
                        }
                    }
                }
                break;
            }
        }
        if (radio_interface < 0 || configuration_altsetting < 0 ||
            promiscuous_altsetting < 0)
        {
            throw std::runtime_error(
                "Wrong USB descriptors (protocol mismatch between burner "
                "module and software?).");
        }
    }

    // Move the dongle into configuration 1 (it will nearly always already be
    // there).
    if (devh.get_configuration() != 1)
    {
        devh.set_configuration(1);
    }

    // Claim the radio interface.
    USB::InterfaceClaimer interface_claimer(devh, 0);

    // Switch to configuration mode.
    devh.set_interface_alt_setting(radio_interface, configuration_altsetting);

    // Set parameters
    {
        std::cout << "Setting channel 0x" << tohex(channel, 2) << "… ";
        std::cout.flush();
        devh.control_no_data(
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
            MRF::CONTROL_REQUEST_SET_CHANNEL, channel,
            static_cast<uint16_t>(radio_interface), 0);
        std::cout << "OK\nSetting symbol rate "
                  << (symbol_rate_encoded ? 625 : 250) << " kb/s… ";
        std::cout.flush();
        devh.control_no_data(
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
            MRF::CONTROL_REQUEST_SET_SYMBOL_RATE, symbol_rate_encoded,
            static_cast<uint16_t>(radio_interface), 0);
        std::cout << "OK\nSetting PAN ID 0x" << tohex(pan_id, 4) << "… ";
        std::cout.flush();
        devh.control_no_data(
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
            MRF::CONTROL_REQUEST_SET_PAN_ID, pan_id,
            static_cast<uint16_t>(radio_interface), 0);
        std::cout << "OK\nSetting MAC address 0x" << tohex(mac_address, 16)
                  << "… ";
        std::cout.flush();
        devh.control_out(
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
            MRF::CONTROL_REQUEST_SET_MAC_ADDRESS, 0,
            static_cast<uint16_t>(radio_interface), &mac_address,
            sizeof(mac_address), 0);
        std::cout << "OK\n";
    }

    // Switch to promiscuous mode. Take a base timestamp, as this event is what
    // causes the dongle to start its internal timestamp counter from zero.
    devh.set_interface_alt_setting(radio_interface, promiscuous_altsetting);
    base_timestamp = get_best_system_timestamp();

    // Set capture flags
    std::cout << "Setting capture flags… ";
    std::cout.flush();
    devh.control_no_data(
        LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
        MRF::CONTROL_REQUEST_SET_PROMISCUOUS_FLAGS, capture_flags,
        static_cast<uint16_t>(radio_interface), 0);
    std::cout << "OK\n";

    // Open the capture file
    std::cout << "Opening capture file… ";
    std::cout.flush();
    std::ofstream ofs;
    ofs.exceptions(
        std::ios_base::eofbit | std::ios_base::failbit | std::ios_base::badbit);
    ofs.open(
        argv[6],
        std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    {
        uint32_t u32;
        uint16_t u16;
        u32 = 0xA1B2C3D4;
        ofs.write(reinterpret_cast<const char *>(&u32), 4);  // Magic number
        u16 = 2;
        ofs.write(
            reinterpret_cast<const char *>(&u16), 2);  // Major version number
        u16 = 4;
        ofs.write(
            reinterpret_cast<const char *>(&u16), 2);  // Minor version number
        u32 = 0;
        ofs.write(
            reinterpret_cast<const char *>(&u32),
            4);  // Timezone offset (we use UTC)
        ofs.write(reinterpret_cast<const char *>(&u32), 4);  // Significant
                                                             // figures in
                                                             // timestamps
                                                             // (everyone sets
                                                             // this to zero)
        u32 = 131;
        ofs.write(
            reinterpret_cast<const char *>(&u32),
            4);  // Maximum length of a captured packet
        u32 = 195;
        ofs.write(
            reinterpret_cast<const char *>(&u32), 4);  // Network data link type
    }
    ofs.flush();
    std::cout << "OK\n";

    // Start accepting capture data.
    std::array<std::unique_ptr<USB::BulkInTransfer>, 32> transfers;
    for (auto &i : transfers)
    {
        i.reset(new USB::BulkInTransfer(devh, 1, 256, false, 0));
        i->signal_done.connect(
            sigc::bind(&handle_transfer_done, std::ref(ofs)));
        i->submit();
    }

    // Go into a main loop.
    MainLoop::run();

    // Report results.
    std::cout << "Captured " << packets_captured << " packets.\n";
    devh.mark_shutting_down();
    return 0;
}

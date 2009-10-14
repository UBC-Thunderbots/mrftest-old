#include "ai/navigator/testnavigator.h"
#include "world/field.h"

testnavigator::testnavigator(player::ptr player, field::ptr field) : 
  navigator(player, field), destInitialized(false)
{

}

void testnavigator::update() {
  //tell it which way to go
  if(destInitialized)
    {
      
      point nowDest = currDest;//this we can change when we have the ball
      //without changing the destination, so if we get/pass the ball
      //on the way to the destination, it goes to where we told it
      
      
    }
}

void testnavigator::go_to_point(const point &destination) {
  //set new destinatin point
  currDest = destination;
  destInitialized = true;
  if(currDest.x > the_field->width()/2)
    currDest.x = the_field->width()/2;
  if(currDest.x < -the_field->width()/2)
    currDest.x = -the_field->width()/2;
  if(currDest.y > the_field->length()/2)
    currDest.y = the_field->length()/2;
  if(currDest.y < -the_field->length()/2)
     currDest.y = the_field->length()/2;
}


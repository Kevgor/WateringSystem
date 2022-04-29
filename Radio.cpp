#include "radio.h"

extern const int RFCommRXPin;

void InitializeRadio() 
{
  // Initialise RF Receiver, the IO and ISR
  vw_set_ptt_inverted(true);  // Required for DR3100
  vw_setup(2000);             // Bits per sec
  vw_set_rx_pin(RFCommRXPin);
  vw_rx_start();        
}

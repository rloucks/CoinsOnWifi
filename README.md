# CoinsOnWifi
NodeMCU / ESP8266 WS2812 LED Matrix Cryptocurrency Ticker 


 -----[ Coins on Wifi ]----------------------------------    
 This uses an crypto stock API, BearSSL, ESP8266, lights     
 solder and time.                                            
                                                             
 Segments are created, and then each light is added based    
 on the call being made to the doL(letter) and doNum(number) 
 you can edit the letters/numbers as you like to create the  
 font look you desire. each segment is based on a 3x5 square 
 in order to achive a long board and use only 100 lights.    
                                                             
 You can technically make a bigger segments, but the segment  
 will look as such, using leds[ XY(col + segX, row)]         
 ------------------------------------------------------------
 0 1 2   (Row 0)      o o o                                  
 0 1 2   (Row 1)      o . o                                  
 0 1 2   (Row 2)      o o o   Makes an A                      
 0 1 2   (Row 3)      o . o                                  
 0 1 2   (Row 4)      o . o                                  


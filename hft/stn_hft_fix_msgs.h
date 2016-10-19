/******************************************************************************************************************

 Author      Date               Comments
 ------      -----              ---------
   mt       09/24/2016          Seperated fix messgae functionality into new file from the op-core

*******************************************************************************************************************/

#ifndef __STN_HFT_FIX_MSGS_H_
#define __STN_HFT_FIX_MSGS_H_




	static uint8_t *FIX_4_2_CheckSumTemplate = "%s10=%s%c";


	static uint8_t *FIX_4_2_Generic_Msg_Template="8=FIX.4.2%c"
												 "9=%04u%c"
												 "35=%c%c"			 /*Message type*/
												 "34=%04u%c"		 /*Sequence number*/ // always encode atleast 4 digits
												 "49=%s%c"			 /*sender comp id*/
												 "52=%s%c"			 /*time */
												 "56=%s";			  /*target comp id*/
												/*doesnt have last 0x01, encoding in here. Because that will be fixed in the message 
																			send*/
												 
	
	
	// LOGIN message type is ='A'
	static uint8_t *FIX_4_2_Login_Template = 
										"57=ADMIN%c"  // -- TargetSubID.  "ADMIN" reserved for administrative messages not intended for a specific user | 
										"90=%u%c"	  // | -- Encrptd Stream Length
										"91=%s%c"	  // 02B4E55C82083A3114A084D6BF984DF69DF845D19484761B |  -- Actual Encrypted Stream
										"95=%u%c"	  //11 – RawDataLength| 
										"96=%s%c"	  // 13641,13640 |	-- Raw Data
										"98=0%c"	  //| -- Encryption Method
										"108=50%c"	  // | -- HeartBtInterval (SECONDS)
										"141=Y%c"	 // |  -- ResetSeqNumFlag, if both sides should reset or not
										"554=%s%c";  // 554 password encoding in clear text
	
	static uint8_t *FIX_4_2_Login_Template_without_Tag90 = 
										"57=ADMIN%c"  // -- TargetSubID.  "ADMIN" reserved for administrative messages not intended for a specific user | 
										"95=%u%c"	  //11 – RawDataLength| 
										"96=%s%c"	  // 13641,13640 |	-- Raw Data
										"98=0%c"	  //| -- Encryption Method
										"108=50%c"	  // | -- HeartBtInterval (SECONDS)
										"141=Y%c"	 // |  -- ResetSeqNumFlag, if both sides should reset or not
										"554=%s%c";  // 554 password encoding in clear text
	
	static uint8_t *FIX_4_2_Login_Template_change_password = 
										"57=ADMIN%c"  // -- TargetSubID.  "ADMIN" reserved for administrative messages not intended for a specific user | 
										"90=%u%c"	  // | -- Encrptd Stream Length
										"91=%s%c"	  // 02B4E55C82083A3114A084D6BF984DF69DF845D19484761B |  -- Actual Encrypted Stream
										"95=%u%c"	  //11 – RawDataLength| 
										"96=%s%c"	  // 13641,13640 |	-- Raw Data
										"98=0%c"	  //| -- Encryption Method
										"108=50%c"	  // | -- HeartBtInterval (SECONDS)
										"141=Y%c"	 // |  -- ResetSeqNumFlag, if both sides should reset or not
										"554=%s%c"  // 554 password encoding in clear text
										"925=%s%c"; // 925 - new password in clear text
	
	static uint8_t *FIX_4_2_Login_Template_without_Tag90_change_password = 
										"57=ADMIN%c"  // -- TargetSubID.  "ADMIN" reserved for administrative messages not intended for a specific user | 
										"95=%u%c"	  //11 – RawDataLength| 
										"96=%s%c"	  // 13641,13640 |	-- Raw Data
										"98=0%c"	  //| -- Encryption Method
										"108=50%c"	  // | -- HeartBtInterval (SECONDS)
										"141=Y%c"	 // |  -- ResetSeqNumFlag, if both sides should reset or not
										"554=%s%c"  // 554 password encoding in clear text
										"925=%s%c"; // 925 - new password in clear text
	
	static uint8_t *FIX_4_2_Order_Entry_Template = 
										"21=%c%c"		// Instructions for order handling on Broker trading floor 
										"11=%s%c"		// Client Order Id
										"38=%d%c"		// Order Qty
										"40=%c%c"		// Order Type
										"44=%u%c"		// Price
										"48=%s%c"		// Security Id Instrument COde, CUSIP ID
										"54=%c%c"		// Side
										"59=0%c"		// Time in Force -- always 0
										"60=%s%c"		// Time of execution/order creation Transaction Time e.g 20110322-10:56:40.882
										"204=1%c"		// Customer or Firm.. Always 1 i.e. Firm
										"%s";		 // 9227=%s%c 15 digit terminal info from MCX Avail a' priori
	
	static uint8_t *FIX_4_2_Order_Entry_Template_with_Account = 
										"1=%s%c"		// account 
										"21=%c%c"		// Instructions for order handling on Broker trading floor 
										"11=%s%c"		// Client Order Id
										"38=%d%c"		// Order Qty
										"40=%c%c"		// Order Type
										"44=%u%c"		// Price
										"48=%s%c"		// Security Id Instrument COde, CUSIP ID
										"54=%c%c"		// Side
										"59=0%c"		// Time in Force -- always 0
										"60=%s%c"		// Time of execution/order creation Transaction Time e.g 20110322-10:56:40.882
										"204=0%c"		// Customer or Firm.. Always 1 i.e. Firm
										"%s";		 // 9227=%s%c15 digit terminal info from MCX Avail a' priori
	
	static uint8_t *FIX_4_2_Order_Replace_Template = 
										"11=%s%c"		// Client Side Order Id
										"21=%c%c"		// Instructions for Order Handling
										"37=%s%c"		// Order Id
										"38=%d%c"		// Order Qty
										"40=%c%c"		// Order Type
										"41=%s%c"		// Orig ClorId
										"44=%u%c"		// Price
										"59=0%c"		// Time in Force -- always 0
										"%s";			// 60=%s%c15 digit terminal info from MCX Avail a' priori
	
	
	static uint8_t *FIX_4_2_Order_Cancel_Template = 
										"11=%s%c"		// Client Side Order Id
										"37=%s%c"		// Order Id
										"41=%s%c"		// Orig ClorId
										"%s";			// 60s=%s%c15 digit terminal info from MCX Avail a' priori
	


#endif


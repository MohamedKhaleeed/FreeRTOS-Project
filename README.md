# FreeRTOS-Project
An embedded code that contain three tasks (two sender and one receiver tasks) communicate via a queue of fixed size where:
-	Each sender task sleeps for a changeable period of time then sends a message to the queue if it is not full.
-	The receiver task sleeps for a fixed period of time then receives one message at a time from the queue.
-	When the number of sent messages reaches 500 messages the program will count and print the number of received messages and the number of blocked messages.   
-	Then the functions reset and clear the queue and repeat again with different sleep time for sender tasks.

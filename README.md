# Networking-protocols
Work done for CS_438 Communication netwoks at UIUC
* MP1: Simple client and server supporting HTTP protocol
* MP2: A TCP-like reliable transport protocol with the following properties:
  - ustilize UDP, some functions are build around it to ensure data are reliably transported between sender and receiver
  - Start by only sending a small amount of data to avoid crapy connection/congestion
  - Build up transmitting speed if connection between sender and receiver looks promising
  - After some threashold is reached, slow down the growth rate of transmitting speed to avoid overflooding the network
  - Slows down again if there is a lost of packet or the order of sending/receiving is not in agreeemnt
* MP3: 2 Network Routing protocol: Link state and Distance Vector

Connect is a C++ based implementation of Chord P2P DHT protocol.

Connect is a terminal application which can be used for file sharing over the
internet.

Client Spec-

1. Port - 5000
2. IP - Currently manual input (but should be automatically detected, IPv4)
3. Base Directory - Automatic (if doesn't exist create - ~/connect_files)
4. 

Interface-

1. Search - search "title" , search for a file
2. Push - push "location"  , share a file
3. Remove - remove "title" , remove a uploaded file
4. Exit - exit
5. Pull - pull "title" , download a file with full name title
6. Show - show , shows the fingertable at the node
7. Stablize - stablize , manually calls the stablize algo.

Request Format- IP`Command`Parameters - Separated by backticks

Search- IP`search`TITLEHASH
PUSH- IP`push`

random

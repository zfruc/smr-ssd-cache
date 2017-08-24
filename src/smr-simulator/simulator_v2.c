/*
  This is a updated versoin (v2) based on log-simulater.
  There are two most important new designs:
  1. FIFO Write adopts APPEND_ONLY, instead of in-place updates.
  2. When the block choosing to evict out of FIFO is a "old version", 
     then drop it off，won't activate write-back, and move head pointer to deal with next one.
*/

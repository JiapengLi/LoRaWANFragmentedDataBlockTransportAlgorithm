# LoRaWAN Fragmented Data Block Transport Algorithm Implementation in C

This library implements forward error correction algorithm of `LoRaWAN Fragmented Data Block Transport Specification v1.0.0`.

https://lora-alliance.org/sites/default/files/2018-09/fragmented_data_block_transport_v1.0.0.pdf

## Compile

### Codeblocks

The library contains a [Codeblocks](http://www.codeblocks.org/) project. You could download Codeblocks from its official website to compile and run the test example.

## Documentation

Check my blog: <https://jiapeng.me/lorawan-fragmented-data-block-transport-algorithm/> (in Chinese)

## History

The original implementation is provided by ARMmbed or Semtech in below mentioned link. I rewrite the code in C (Originally provided in C++). The optimization is to use bitmap instead of buffer to reduce memory cost, and of course it will run slower than the ARMmbed or Semtech library. 

https://github.com/ARMmbed/mbed-lorawan-update-client/blob/master/fragmentation/source/FragmentationMath.cpp

## Reference

- <https://github.com/ARMmbed/mbed-lorawan-update-client/blob/master/fragmentation/source/FragmentationMath.cpp>
- https://github.com/brocaar/lorawan/blob/master/applayer/fragmentation/encode.go
- https://github.com/brocaar/lorawan/blob/master/applayer/fragmentation/encode_test.go
- https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
- https://www.geeksforgeeks.org/count-set-bits-in-an-integer/
jobs=1

./libvlc_decoder_fuzzer -jobs=$jobs --codec=H264 DECODER_CORPUS/H264
./libvlc_decoder_fuzzer -jobs=$jobs --codec=H265 DECODER_CORPUS/H265
./libvlc_decoder_fuzzer -jobs=$jobs --codec=MP2V DECODER_CORPUS/MP2V
./libvlc_decoder_fuzzer -jobs=$jobs --codec=MP4V DECODER_CORPUS/MP4V
./libvlc_decoder_fuzzer -jobs=$jobs --codec=VC-1 DECODER_CORPUS/VC-1

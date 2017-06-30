#define ZFP_TYPE zfp_type_int64
#define DIM_INT_STR 3dInt64

// single block
#define ZFP_RATE_PARAM_BITS 19
#define ZFP_PREC_PARAM_BITS 22

#define CHECKSUM_ORIGINAL_DATA_BLOCK 0xcc5133515849571c
#define CHECKSUM_ENCODED_BLOCK 0x6c0ff959c2207d41
#define CHECKSUM_ENCODED_PARTIAL_BLOCK 0xd6b771a93e2404f4

#define CHECKSUM_DECODED_BLOCK 0x4b9f52d500000000
#define CHECKSUM_DECODED_PARTIAL_BLOCK 0xc78c8cca00000000

// entire array
#define CHECKSUM_ORIGINAL_DATA_ARRAY 0x22aaf6fabab92a02

#define CHECKSUM_FP_COMPRESSED_BITSTREAM_0 0x31965cf9b957853e
#define CHECKSUM_FP_COMPRESSED_BITSTREAM_1 0x1e83bc9dfb6caadc
#define CHECKSUM_FP_COMPRESSED_BITSTREAM_2 0xa51f0f1379fb6866
#define CHECKSUM_FP_DECOMPRESSED_ARRAY_0 0xd9ffdafa00000000
#define CHECKSUM_FP_DECOMPRESSED_ARRAY_1 0xc05a9e9b00000000
#define CHECKSUM_FP_DECOMPRESSED_ARRAY_2 0xacb9720c31470a11

#define CHECKSUM_FR_COMPRESSED_BITSTREAM_0 0x964e6d9d99cf8820
#define CHECKSUM_FR_COMPRESSED_BITSTREAM_1 0x64cc7e448ca2dd90
#define CHECKSUM_FR_COMPRESSED_BITSTREAM_2 0x25a4cbb08f12494f
#define CHECKSUM_FR_DECOMPRESSED_ARRAY_0 0xc09a2ddd00000000
#define CHECKSUM_FR_DECOMPRESSED_ARRAY_1 0x1ecf957ee780a6bd
#define CHECKSUM_FR_DECOMPRESSED_ARRAY_2 0xb00e5f04de7a4dd9

// unused but necessary for compilation
#define CHECKSUM_FA_COMPRESSED_BITSTREAM_0 0
#define CHECKSUM_FA_COMPRESSED_BITSTREAM_1 0
#define CHECKSUM_FA_COMPRESSED_BITSTREAM_2 0
#define CHECKSUM_FA_DECOMPRESSED_ARRAY_0 0
#define CHECKSUM_FA_DECOMPRESSED_ARRAY_1 0
#define CHECKSUM_FA_DECOMPRESSED_ARRAY_2 0

#include "array/zfparray3.h"
using namespace zfp;

extern "C" {
  #include "constants/3dDouble.h"
  #include "utils/hash64.h"
};

#include "gtest/gtest.h"
#include "utils/gtestDoubleEnv.h"
#include "utils/gtestBaseFixture.h"
#include "utils/predicates.h"

class Array3dTestEnv : public ArrayDoubleTestEnv {
public:
  virtual int getDims() { return 3; }
};

class Array3dTest : public ArrayNdTestFixture {};

TEST_F(Array3dTest, when_constructorCalled_then_rateSetWithWriteRandomAccess)
{
  double rate = ZFP_RATE_PARAM_BITS;
  array3d arr(inputDataSideLen, inputDataSideLen, inputDataSideLen, rate);
  // wra in 3D supports integer fixed-rates [1, 64] (use <=)
  EXPECT_LE(rate, arr.rate());
}

TEST_F(Array3dTest, when_setRate_then_compressionRateChanged)
{
  double oldRate = ZFP_RATE_PARAM_BITS;
  array3d arr(inputDataSideLen, inputDataSideLen, inputDataSideLen, oldRate, inputDataArr);

  double actualOldRate = arr.rate();
  size_t oldCompressedSize = arr.compressed_size();
  uint64 oldChecksum = hashBitstream((uint64*)arr.compressed_data(), oldCompressedSize);

  double newRate = oldRate - 10;
  EXPECT_LT(1, newRate);
  arr.set_rate(newRate);
  EXPECT_GT(actualOldRate, arr.rate());

  arr.set(inputDataArr);
  size_t newCompressedSize = arr.compressed_size();
  uint64 checksum = hashBitstream((uint64*)arr.compressed_data(), newCompressedSize);

  EXPECT_PRED_FORMAT2(ExpectNeqPrintHexPred, oldChecksum, checksum);

  EXPECT_GT(oldCompressedSize, newCompressedSize);
}

TEST_F(Array3dTest, when_generateRandomData_then_checksumMatches)
{
  EXPECT_PRED_FORMAT2(ExpectEqPrintHexPred, CHECKSUM_ORIGINAL_DATA_ARRAY, hashArray((uint64*)inputDataArr, inputDataTotalLen, 1));
}

INSTANTIATE_TEST_CASE_P(TestManyCompressionRates, Array3dTest, ::testing::Values(0, 1, 2));

TEST_P(Array3dTest, given_dataset_when_set_then_underlyingBitstreamChecksumMatches)
{
  array3d arr(inputDataSideLen, inputDataSideLen, inputDataSideLen, getRate());

  uint64 expectedChecksum = getExpectedBitstreamChecksum();
  uint64 checksum = hashBitstream((uint64*)arr.compressed_data(), arr.compressed_size());
  EXPECT_PRED_FORMAT2(ExpectNeqPrintHexPred, expectedChecksum, checksum);

  arr.set(inputDataArr);

  checksum = hashBitstream((uint64*)arr.compressed_data(), arr.compressed_size());
  EXPECT_PRED_FORMAT2(ExpectEqPrintHexPred, expectedChecksum, checksum);
}

TEST_P(Array3dTest, given_setArray3d_when_get_then_decompressedValsReturned)
{
  array3d arr(inputDataSideLen, inputDataSideLen, inputDataSideLen, getRate());
  arr.set(inputDataArr);

  double* decompressedArr = new double[inputDataTotalLen];
  arr.get(decompressedArr);

  uint64 expectedChecksum = getExpectedDecompressedChecksum();
  uint64 checksum = hashArray((uint64*)decompressedArr, inputDataTotalLen, 1);
  EXPECT_PRED_FORMAT2(ExpectEqPrintHexPred, expectedChecksum, checksum);

  delete[] decompressedArr;
}

TEST_P(Array3dTest, given_populatedCompressedArray_when_resizeWithClear_then_bitstreamZeroed)
{
  array3d arr(inputDataSideLen, inputDataSideLen, inputDataSideLen, getRate());
  arr.set(inputDataArr);
  EXPECT_NE(0, hashBitstream((uint64*)arr.compressed_data(), arr.compressed_size()));

  arr.resize(inputDataSideLen + 1, inputDataSideLen - 1, inputDataSideLen + 3, true);

  EXPECT_EQ(0, hashBitstream((uint64*)arr.compressed_data(), arr.compressed_size()));
}

TEST_P(Array3dTest, when_configureCompressedArrayFromDefaultConstructor_then_bitstreamChecksumMatches)
{
  array3d arr;
  arr.resize(inputDataSideLen, inputDataSideLen, inputDataSideLen, false);
  arr.set_rate(getRate());
  arr.set(inputDataArr);

  uint64 expectedChecksum = getExpectedBitstreamChecksum();
  uint64 checksum = hashBitstream((uint64*)arr.compressed_data(), arr.compressed_size());
  EXPECT_PRED_FORMAT2(ExpectEqPrintHexPred, expectedChecksum, checksum);
}

void CheckMemberVarsCopied(array3d arr1, array3d arr2)
{
  double oldRate = arr1.rate();
  size_t oldCompressedSize = arr1.compressed_size();
  size_t oldSizeX = arr1.size_x();
  size_t oldSizeY = arr1.size_y();
  size_t oldSizeZ = arr1.size_z();
  size_t oldSize = arr1.size();
  size_t oldCacheSize = arr1.cache_size();

  arr1.set_rate(oldRate + 10);
  arr1.resize(oldSizeX - 10, oldSizeY - 10, oldSizeZ - 10);
  arr1.set(inputDataArr);
  arr1.set_cache_size(oldCacheSize + 10);

  EXPECT_EQ(oldRate, arr2.rate());
  EXPECT_EQ(oldCompressedSize, arr2.compressed_size());
  EXPECT_EQ(oldSizeX, arr2.size_x());
  EXPECT_EQ(oldSizeY, arr2.size_y());
  EXPECT_EQ(oldSizeZ, arr2.size_z());
  EXPECT_EQ(oldSize, arr2.size());
  EXPECT_EQ(oldCacheSize, arr2.cache_size());
}

void CheckDeepCopyPerformed(array3d arr1, array3d arr2, uchar* arr1UnflushedBitstreamPtr)
{
  // flush arr2 first, to ensure arr1 remains unflushed
  uint64 checksum = hashBitstream((uint64*)arr2.compressed_data(), arr2.compressed_size());
  uint64 arr1UnflushedChecksum = hashBitstream((uint64*)arr1UnflushedBitstreamPtr, arr1.compressed_size());
  EXPECT_PRED_FORMAT2(ExpectNeqPrintHexPred, arr1UnflushedChecksum, checksum);

  // flush arr1, compute its checksum, clear its bitstream, re-compute arr2's checksum
  uint64 expectedChecksum = hashBitstream((uint64*)arr1.compressed_data(), arr1.compressed_size());
  arr1.resize(arr1.size_x(), arr1.size_y(), arr1.size_z());
  checksum = hashBitstream((uint64*)arr2.compressed_data(), arr2.compressed_size());
  EXPECT_PRED_FORMAT2(ExpectEqPrintHexPred, expectedChecksum, checksum);
}

TEST_P(Array3dTest, given_compressedArray_when_copyConstructor_then_memberVariablesCopied)
{
  array3d arr(inputDataSideLen, inputDataSideLen, inputDataSideLen, getRate(), inputDataArr, 128);

  array3d arr2(arr);

  CheckMemberVarsCopied(arr, arr2);
}

TEST_P(Array3dTest, given_compressedArray_when_copyConstructor_then_deepCopyPerformed)
{
  // create arr with dirty cache
  array3d arr(inputDataSideLen, inputDataSideLen, inputDataSideLen, getRate(), inputDataArr);
  uchar* arrUnflushedBitstreamPtr = arr.compressed_data();
  arr[0] = 999;

  array3d arr2(arr);

  CheckDeepCopyPerformed(arr, arr2, arrUnflushedBitstreamPtr);
}

TEST_P(Array3dTest, given_compressedArray_when_setSecondArrayEqualToFirst_then_memberVariablesCopied)
{
  array3d arr(inputDataSideLen, inputDataSideLen, inputDataSideLen, getRate(), inputDataArr, 128);

  array3d arr2 = arr;

  CheckMemberVarsCopied(arr, arr2);
}

TEST_P(Array3dTest, given_compressedArray_when_setSecondArrayEqualToFirst_then_deepCopyPerformed)
{
  // create arr with dirty cache
  array3d arr(inputDataSideLen, inputDataSideLen, inputDataSideLen, getRate(), inputDataArr);
  uchar* arrUnflushedBitstreamPtr = arr.compressed_data();
  arr[0] = 999;

  array3d arr2 = arr;

  CheckDeepCopyPerformed(arr, arr2, arrUnflushedBitstreamPtr);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::Environment* const foo_env = ::testing::AddGlobalTestEnvironment(new Array3dTestEnv);
  return RUN_ALL_TESTS();
}

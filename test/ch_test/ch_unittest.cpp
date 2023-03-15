// Step 1. Include necessary header files such that the stuff your
// test logic needs is declared.
//
// Don't forget gtest.h, which declares the testing framework.


#include "less.hpp"

#include "ch_mocks.hpp"
#include "gmock/gmock.h" // Brings in gMock.
#include "gtest/gtest.h"

#include "ch.cpp"

using namespace ::testing;
using namespace ch;

// helper to copy a filename string
char* copyString(const char* theString)
{
    int len = strlen(theString);
    char* copiedString = (char*)calloc(len + 1, sizeof(char));
    memcpy(copiedString, theString, len);
    return copiedString;
}

struct Mocks
    : public ::testing::StrictMock<ChMock> {
};

template <typename T>
struct ModuleTest : public ::testing::Test {
protected:
    ModuleTest()
    {
        moduleTestMocks.reset(new T);
    }
    virtual ~ModuleTest()
    {
        moduleTestMocks.reset();
    }
    void SetUp()
    {
    }
    void TearDown()
    {
    }
};

struct TestFixture : public ModuleTest<Mocks> {
};

// --------------------------------------------------------------
class ChUnitTestEmpty : public TestFixture {
public:
    ChUnitTestEmpty()
    {
      less::Settings::autobuf = 0;
      less::Settings::logfile = -1;
      less::Settings::follow_mode = 0;   
      curr_ino = 0;
      curr_dev = 0;
      screen_trashed = TRASHED;
      less::Settings::sigs = 0;
      less::Settings::namelogfile = NULL;
    }

    filestate *capturedFilestate = nullptr;

    void TearDown()
    {
      if (! (capturedFilestate == nullptr))
        free(capturedFilestate);
    }


    void captureFilestate(void * obj){
      capturedFilestate = (filestate *)obj;
    }

};
// --------------------------------------------------------------

TEST_F(ChUnitTestEmpty, chflags)
{
  int res = getflags();
  EXPECT_EQ(res, 0);
}

TEST_F(ChUnitTestEmpty, tell_nullptr)
{
  position_t pos = tell();
  EXPECT_EQ(pos, NULL_POSITION);
}

TEST_F(ChUnitTestEmpty, ch_nullptr_tests)
{
  close();    

  int res = ch_get();      
  EXPECT_EQ(res, EOI);

  res = seek(1);
  EXPECT_EQ(res, 0);

  res = end_seek();
  EXPECT_EQ(res, 0);

  res = end_buffer_seek();    
  EXPECT_EQ(res, 0);
  
  position_t pos = length();
  EXPECT_EQ(pos, NULL_POSITION);

  res = forw_get();
  EXPECT_EQ(res, EOI);

  res = back_get();
  EXPECT_EQ(res, EOI);

  flush();
    
}

TEST_F(ChUnitTestEmpty, init1)
{
  const int test_file_id = 10;
  const int test_flags = 0;

  using namespace ifile;
  Ifile * lclIfile = new Ifile(nullptr);

  EXPECT_CALL(GetMock<ChMock>(), getCurrentIfile()).
    Times(2).
    WillOnce(Return(lclIfile)).
    WillOnce(Return(lclIfile));
  
  EXPECT_CALL(*lclIfile, getFilestate()).Times(1).
    WillOnce(Return(nullptr));
  
  // This will capture the filestate object being saved
  EXPECT_CALL(*lclIfile, setFilestate(_)).
    Times(1).
    WillOnce(DoAll(Invoke(this, &ChUnitTestEmpty::captureFilestate), Return()));

  init(test_file_id, 
          test_flags);

  EXPECT_EQ(capturedFilestate->file,  test_file_id);
  EXPECT_EQ(capturedFilestate->flags, test_flags);

  delete lclIfile;
}

TEST_F(ChUnitTestEmpty, init2)
{

  const int test_file_id = 10;
  const int test_flags = CH_CANSEEK | CH_HELPFILE;

  using namespace ifile;
  Ifile * lclIfile = new Ifile(nullptr);

  EXPECT_CALL(GetMock<ChMock>(), getCurrentIfile()).
    Times(2).
    WillOnce(Return(lclIfile)).
    WillOnce(Return(lclIfile));
  
  EXPECT_CALL(*lclIfile, getFilestate()).Times(1).
    WillOnce(Return(nullptr));
  
  EXPECT_CALL(GetMock<ChMock>(), lseek(test_file_id, _, _)).
    Times(1).
    WillOnce(Return(BAD_LSEEK)); // is Seekable return - false
    
  // This will capture the filestate object being saved
  EXPECT_CALL(*lclIfile, setFilestate(_)).
    Times(1).
    WillOnce(DoAll(Invoke(this, &ChUnitTestEmpty::captureFilestate), Return()));
  
  // also covers seekable
  init(test_file_id,
          test_flags); 

  EXPECT_EQ(capturedFilestate->file, test_file_id);
  EXPECT_EQ(capturedFilestate->flags, test_flags & ~CH_CANSEEK);

  delete lclIfile;
  
}

TEST_F(ChUnitTestEmpty, init3)
{

  const int test_file_id = 10;
  const int test_flags = CH_KEEPOPEN;

  using namespace ifile;
  Ifile * lclIfile = new Ifile(nullptr);

  EXPECT_CALL(GetMock<ChMock>(), getCurrentIfile()).
    Times(2).
    WillOnce(Return(lclIfile)).
    WillOnce(Return(lclIfile));
  
  EXPECT_CALL(*lclIfile, getFilestate()).Times(1).
    WillOnce(Return(nullptr));
  
  // This will capture the filestate object being saved
  EXPECT_CALL(*lclIfile, setFilestate(_)).
    Times(1).
    WillOnce(DoAll(Invoke(this, &ChUnitTestEmpty::captureFilestate), Return()));

  init(test_file_id, CH_KEEPOPEN); 

  EXPECT_EQ(capturedFilestate->file, test_file_id);
  EXPECT_EQ(capturedFilestate->flags, test_flags);

  // Part 2 == getflags
  int getflags_res = getflags();
  EXPECT_EQ(getflags_res, test_flags);

  delete lclIfile;
}

TEST_F(ChUnitTestEmpty, ch_setbuffspace)
{

  EXPECT_EQ(maxbufs, -1);

  setbufspace(-100); 

  EXPECT_EQ(maxbufs, -1);

  setbufspace(1); 

  EXPECT_EQ(maxbufs, 1);

  setbufspace(1000); 

  EXPECT_EQ(maxbufs, 125);

  setbufspace(100000); 

  EXPECT_EQ(maxbufs, 12500);
}

// --------------------------------------------------------------
class ChUnitTest1Buff : public TestFixture {
public:
    ifile::Ifile* lclIfile = nullptr;
    filestate* capturedFilestate = nullptr;

    void captureFilestate(void* obj)
    {
        capturedFilestate = (filestate*)obj;
    }

    const int default_file_id = 1;
    const int default_flags = 0;
    const int default_fsize = 1024;
    const int default_fpos = 10;

    ChUnitTest1Buff()
    {
        log("Fixture constructor", false);
        less::Settings::autobuf = 0;
        less::Settings::logfile = -1;
        less::Settings::follow_mode = 0;
        curr_ino = 0;
        curr_dev = 0;
        screen_trashed = TRASHED;
        less::Settings::sigs = 0;
        less::Settings::namelogfile = nullptr;
        lclIfile = new ifile::Ifile(nullptr);
    }

    void SetUp()
    {

        log("Setup start", false);

        EXPECT_CALL(GetMock<ChMock>(), 
          getCurrentIfile()).
          Times(2).
          WillOnce(Return(lclIfile)).
          WillOnce(Return(lclIfile));

        EXPECT_CALL(*lclIfile, 
          getFilestate()).
          Times(1).
          WillOnce(Return(nullptr));

        EXPECT_CALL(*lclIfile, 
          setFilestate(_)).
          Times(1).
          WillOnce(DoAll(
            Invoke(this, &ChUnitTest1Buff::captureFilestate), 
            Return()));

        init(default_file_id, 
                default_flags); 
        
        thisfile->fsize = default_fsize;
        thisfile->fpos = default_fpos;

        log("Setup end", false);
    }

    void TearDown()
    {
        log("TearDown", false);

        if (! (capturedFilestate == nullptr))
          free(capturedFilestate);

        
        //if (thisfile->nbufs > 0) 
        //  ch_delbufs();

        delete lclIfile;
    }
};
// --------------------------------------------------------------

TEST_F(ChUnitTest1Buff, init_and_tell)
{
  using namespace ifile;

  const int test_file_id    = 9;
  const int test_flags      = CH_NODATA | CH_CANSEEK;
  const int test_file_size  = 9999;
  const int test_block      = 0;
  const int test_offset     = 0;

  capturedFilestate->file  = -1; // Set to null - will change in test
  capturedFilestate->flags = test_flags;
  capturedFilestate->fsize = test_file_size;
  capturedFilestate->block = test_block;
  capturedFilestate->offset = test_offset;

  EXPECT_CALL(GetMock<ChMock>(), 
    getCurrentIfile()).
    Times(1).
    WillOnce(Return(lclIfile)); 

  EXPECT_CALL(*lclIfile, 
    getFilestate()).
    Times(1).
    WillOnce(Return((void*)capturedFilestate));
  
  EXPECT_CALL(GetMock<ChMock>(), 
    lseek(test_file_id, _, _)).
    Times(1).
    WillOnce(Return(1)); // Not BADLSEEK

  EXPECT_CALL(GetMock<ChMock>(), 
    filename::filesize(9)).
    Times(1).
    WillOnce(Return(test_file_size));

  // part 1 - init
  // also covers init_hashtbl, seekable
  init(test_file_id,   
          test_flags);    

  EXPECT_EQ(capturedFilestate->file, test_file_id);
  EXPECT_EQ(capturedFilestate->flags, test_flags); 
  EXPECT_EQ(capturedFilestate->fsize, test_file_size);

  // part 2 == tell
  position_t pos = tell();

  EXPECT_EQ(pos, (test_block * LBUFSIZE) + test_offset);

  // Part 3 == getflags
  int getflags_res = getflags();

  EXPECT_EQ(getflags_res, test_flags);

}

TEST_F(ChUnitTest1Buff, close)
{
  using namespace ifile;

  EXPECT_CALL(GetMock<ChMock>(), close(default_file_id)).
     Times(1).
     WillOnce(Return(-1)); 

  close();    

  EXPECT_NE(thisfile, nullptr);
  EXPECT_EQ(thisfile->file, -1);
}

TEST_F(ChUnitTest1Buff, close_keepstate)
{
  using namespace ifile;
  const int test_flags      =  CH_CANSEEK | CH_KEEPOPEN; // keepopen wont call close
  capturedFilestate->flags = test_flags;

  close();    

  EXPECT_NE(thisfile, nullptr);

}

TEST_F(ChUnitTest1Buff, close_notkeepfilestate)
{
  using namespace ifile;

  const int test_flags      =  CH_CANSEEK; // keepopen wont call close
  
  capturedFilestate->flags = test_flags;

  EXPECT_CALL(GetMock<ChMock>(), close(default_file_id)).
     Times(1).
     WillOnce(Return(-1)); 
  
  EXPECT_CALL(GetMock<ChMock>(), getCurrentIfile()).
    Times(1).
    WillOnce(Return(lclIfile));

  EXPECT_CALL(*lclIfile, setFilestate(nullptr)).
    Times(1).
    WillOnce(Return());

  close();
  capturedFilestate = nullptr;    

  EXPECT_EQ(thisfile, nullptr);

}

TEST_F(ChUnitTest1Buff, set_eof)
{

  EXPECT_EQ(thisfile->fsize, default_fsize);
  EXPECT_EQ(thisfile->fpos, default_fpos);
  EXPECT_NE(thisfile->fpos, thisfile->fsize);

  set_eof();

  EXPECT_EQ(thisfile->fsize, default_fpos);
}

TEST_F(ChUnitTest1Buff, create_and_delete_bufs)
{

  EXPECT_EQ(thisfile->nbufs, 0);
  
  // part1 add buffer
  int res = ch_addbuf();

  EXPECT_EQ(res, 0);
  EXPECT_EQ(thisfile->nbufs, 1);
  
  // part2 add 2nd buffer
  res = ch_addbuf();

  EXPECT_EQ(res, 0);
  EXPECT_EQ(thisfile->nbufs, 2);
  
  // part 3 delete all
  ch_delbufs();
  EXPECT_EQ(thisfile->nbufs, 0);
}

using ::testing::Invoke;

// --------------------------------------------------------------
class ChUnitTestBaseBuff : public TestFixture {
public:
    ifile::Ifile* lclIfile = nullptr;
    filestate* capturedFilestate = nullptr;
    unsigned char * capturedBuffer = nullptr;
    char bufferDataForTest[LBUFSIZE+1];

    void captureFilestate(void* obj)
    {
        capturedFilestate = (filestate*)obj;
    }

    void captureAndUpdateBuffer(Unused, void* buffer, Unused) {
      capturedBuffer = (unsigned char *) buffer;
      
      int savedDataLen = strlen(bufferDataForTest);
      strncpy(
        reinterpret_cast<char *>(capturedBuffer), 
        bufferDataForTest, 
        savedDataLen);

      log("Copied to return buffer: >" + 
        std::string(reinterpret_cast<char *>(capturedBuffer)) +
        "<");
    }


    // Assume file opened has file descriptor 1, file size is 1024 bytes

    const int default_file_id = 1;
    const int default_flags = CH_CANSEEK;
    const int default_fsize = 1024;
    const int default_fpos = 0;
    const int default_logfile_fileid = 2;
    const char * defailt_logfile_filename="logfile.txt";

    ChUnitTestBaseBuff()
    {
        log("Fixture constructor", false);
        less::Settings::autobuf = 0;
        less::Settings::logfile = default_logfile_fileid;
        less::Settings::follow_mode = 0;
        curr_ino = 0;
        curr_dev = 0;
        screen_trashed = TRASHED;
        less::Settings::sigs = 0;
        less::Settings::namelogfile = copyString(defailt_logfile_filename);
        lclIfile = new ifile::Ifile(nullptr);
        
        // Space fill all
        memset(bufferDataForTest, ' ', LBUFSIZE);
        bufferDataForTest[LBUFSIZE] = '\0';
    }

    void SetUp()
    {

        log("Setup start", false);

        EXPECT_CALL(GetMock<ChMock>(), 
          getCurrentIfile()).
          Times(2).
          WillOnce(Return(lclIfile)).
          WillOnce(Return(lclIfile));

        EXPECT_CALL(*lclIfile, 
          getFilestate()).
          Times(1).WillOnce(Return(nullptr));

        EXPECT_CALL(GetMock<ChMock>(), 
          lseek(default_file_id, 1, _)).
          Times(1).
          WillOnce(Return(1)); // Not BADLSEEK

        EXPECT_CALL(GetMock<ChMock>(), 
          lseek(default_file_id, 0, _)).
          Times(2).
          WillRepeatedly(Return(1)); // Not BADLSEEK

        EXPECT_CALL(GetMock<ChMock>(), 
          filename::filesize(default_file_id)).
          Times(2).
          WillRepeatedly(Return(default_fsize));

        EXPECT_CALL(*lclIfile, 
          setFilestate(_)).
          Times(1).
          WillOnce(DoAll(
            Invoke(this, &ChUnitTestBaseBuff::captureFilestate), 
            Return()));

        init(default_file_id, 
                default_flags); 
        
        log("Setup end", false);
    }

    void TearDown()
    {
      log("TearDown", false);

      if (thisfile->nbufs > 0) 
          ch_delbufs();

      free(less::Settings::namelogfile);
      
      if (! (capturedFilestate == nullptr))
          free(capturedFilestate);

      delete lclIfile;
    }
};
// --------------------------------------------------------------

TEST_F(ChUnitTestBaseBuff, repeat_ch_forw_back_gets)
{

  // Read in less than 1 buffer of data.
  int testReadSize = default_fsize;
  char data[] = "ABCDEFG";

  // ensure null terminated
  bufferDataForTest[testReadSize] = '\0';

  // Space fill all
  memset(bufferDataForTest, ' ', testReadSize);

  // Copy string  
  strncpy(bufferDataForTest, data, 7);
  // Copy to the end of the buffer also
  char * buffPtr = &bufferDataForTest[testReadSize-7];
  strncpy(buffPtr, data, 7);
  
  EXPECT_EQ(strlen(bufferDataForTest), testReadSize);


  // expectations for all parts
  EXPECT_CALL(GetMock<ChMock>(), 
     write(default_logfile_fileid, _, _)).
     Times(2).
     WillRepeatedly(Return(0)); // any return will do

  // ================================================================
  // Part 1 - Read 1st char without any seek. Reads in non full buf
  // ================================================================
  
  EXPECT_CALL(GetMock<ChMock>(), 
     iread(default_file_id, _, LBUFSIZE)).
     Times(1).
     WillOnce(DoAll(
            Invoke(this, &ChUnitTestBaseBuff::captureAndUpdateBuffer), 
            Return(testReadSize)));

  int res = forw_get();

  EXPECT_EQ(thisfile->nbufs, 1);
  EXPECT_EQ(res, int(data[0]));
  EXPECT_EQ(thisfile->offset, 1);

  // ================================================================
  // Part 2 - get next char - no write expected
  // ================================================================

  res = forw_get();

  EXPECT_EQ(thisfile->nbufs, 1);
  EXPECT_EQ(res, int(data[1]));
  EXPECT_EQ(thisfile->offset, 2);

  // ================================================================
  // Part 3 - get 3rd character and all the way to the last
  // ================================================================

  // read to the end of the "file"
  for (int i=3; i < testReadSize+1; i++){

    res = forw_get();

    EXPECT_EQ(thisfile->nbufs, 1);
    EXPECT_EQ(res, int(bufferDataForTest[i-1]));
    EXPECT_EQ(thisfile->offset, i);
  }

  // ================================================================
  // Part 4 - attempt read after getting to end of file
  // ================================================================

  res = forw_get();

  EXPECT_EQ(thisfile->nbufs, 1);
  EXPECT_EQ(res, -1);
  EXPECT_EQ(thisfile->offset, testReadSize);
  EXPECT_EQ(tell(), testReadSize);
  EXPECT_EQ(length(), testReadSize);
  
  // ================================================================
  // Part 5 - read back one char
  // ================================================================

  // Read back
  res = back_get();

  EXPECT_EQ(thisfile->nbufs, 1);
  EXPECT_EQ(res, int(bufferDataForTest[testReadSize-1]));
  EXPECT_EQ(thisfile->offset, testReadSize-1);
  EXPECT_EQ(tell(), testReadSize-1);
  EXPECT_EQ(length(), testReadSize);

  // ================================================================
  // Part 6 - read next back char
  // ================================================================

  res = back_get();

  EXPECT_EQ(thisfile->nbufs, 1);
  EXPECT_EQ(res, int(bufferDataForTest[testReadSize-2]));
  EXPECT_EQ(thisfile->offset, testReadSize-2);
  EXPECT_EQ(tell(), testReadSize-2);
  EXPECT_EQ(length(), testReadSize);

  // ================================================================
  // Part 7 = seek to near end of file
  // ================================================================

  // seek near end
  res = seek(testReadSize - 6);

  EXPECT_EQ(res, 0);

  // ================================================================
  // Part 8 - read next char
  // ================================================================

  res = forw_get();

  EXPECT_EQ(thisfile->nbufs, 1);
  EXPECT_EQ(res, int(bufferDataForTest[testReadSize-6]));
  EXPECT_EQ(thisfile->offset, testReadSize-5);
  EXPECT_EQ(tell(), testReadSize-5);
  EXPECT_EQ(length(), testReadSize);

  // ================================================================
  // Part 9 - see to start of file again
  // ================================================================

  res = beg_seek();

  EXPECT_EQ(res, 0);
  EXPECT_EQ(thisfile->offset, 0);
  EXPECT_EQ(tell(), 0);

  // ================================================================
  // Part 10 - read 1st char again
  // ================================================================

  // get 1st char in file
  res = forw_get();

  EXPECT_EQ(thisfile->nbufs, 1);
  EXPECT_EQ(res, int(bufferDataForTest[0]));
  EXPECT_EQ(thisfile->offset, 1);
  EXPECT_EQ(tell(), 1);
  EXPECT_EQ(length(), testReadSize);

  // ================================================================
  // Part 11 - check if block is buffered - 1st char
  // ================================================================

  bool boolRes = buffered(0);
  EXPECT_EQ(boolRes, true);

  // ================================================================
  // Part 12 - check if non existing block is buffered
  // ================================================================

  boolRes = buffered(99);
  EXPECT_EQ(boolRes, false);

  // ================================================================
  // Part 13 - sync logfile
  // ================================================================

  EXPECT_STREQ(less::Settings::namelogfile, defailt_logfile_filename);
  sync_logfile();  

  // ================================================================
  // Part 14 - close logfile
  // ================================================================
 
  EXPECT_CALL(GetMock<ChMock>(), 
     close(default_logfile_fileid)).
     Times(1).
     WillRepeatedly(Return(0)); // any return will do

  end_logfile();  
  EXPECT_EQ(less::Settings::namelogfile, nullptr);

  // ================================================================
  // Part 14 - cleanup
  // ================================================================

  flush();
  ch_delbufs();

  EXPECT_EQ(thisfile->nbufs, 0);
  // TODO - more tests for delbufs and flush

}

// --------------------------------------------------------------
class ChUnitTestBaseBigFile : public TestFixture {
public:

  std::exception invalid_setup_data;

  ifile::Ifile* lclIfile = nullptr;
  filestate* capturedFilestate = nullptr;
  unsigned char * capturedBuffer = nullptr;
  char bufferDataForTest[LBUFSIZE+1];

  std::map<int, buf *> savedBufMap; // block index
  std::vector<buf *> savedBufVector;

  int currentBlock = 0;
  bool skipCaptureAndUpdateBuffer = false;

  void captureFilestate(void* obj)
  {
      capturedFilestate = (filestate*)obj;
  }

  /*
   * createBufferForBlock - for the given block number create data to provide
   * in the test as data read from the file for that block
   */
  void createBufferForBlock(int blockNum) {

    std::string s;

    if (blockNum < 10)
      s = std::to_string(blockNum) + "BCDEFG";
    else if (blockNum < 100)
      s = std::to_string(blockNum) + "CDEFG";
    else if (blockNum < 1000) 
      s = std::to_string(blockNum) + "DEFG";
    else if (blockNum < 10000) 
      s = std::to_string(blockNum) + "EFG";

    //char default_data[7] = s.c_str();

    buf * currentBuf = new (buf);

    currentBuf->datasize = LBUFSIZE;

    memset(currentBuf->data, ' ', LBUFSIZE);
    currentBuf->data[LBUFSIZE-1] = '\0';

    strncpy(reinterpret_cast<char *>(currentBuf->data), s.c_str(), 7); 

    // Copy to the end of the buffer also
    
    char * bufPtr = reinterpret_cast<char *>(&currentBuf->data[LBUFSIZE-7]);
    strncpy(bufPtr, s.c_str(), 7);
    
    savedBufMap[blockNum] = currentBuf;
    savedBufVector.push_back(currentBuf);
  }

  /* 
   * bufForBlock - for the given block number return saved data 
   * for that block
   */
  buf * bufForBlock (int blockNum){
    auto res = savedBufMap.find(blockNum);
    if (res != savedBufMap.end()) 
      return res->second;
    else 
      throw invalid_setup_data;
  }


  /*
   * Capture for iread call
   * Capture the buffer in iread call and copy the saved
   * data for the current block to it
   */
  void captureAndUpdateBuffer(Unused, void* buffer, unsigned int len) {
        
    if (!skipCaptureAndUpdateBuffer) {
      capturedBuffer = (unsigned char *) buffer;
      
      buf * currentBuf = bufForBlock(currentBlock);
          
      strncpy(
        reinterpret_cast<char *>(capturedBuffer), 
        reinterpret_cast<char *>(currentBuf->data),
        len);

      log("Copied block " + std::to_string(currentBlock) +
        " to return buffer: >" + 
        std::string(reinterpret_cast<char *>(capturedBuffer)) +
        "<");
    }
  }


  // Assume file opened has file descriptor 1, file size is 1024 bytes

  const int default_file_id = 1;
  const int default_flags = CH_CANSEEK;
  const int default_fsize = LBUFSIZE * 100; // 100 buffers in size
  const int default_fpos = 0;
  const int default_logfile_fileid = 2;
  const char * default_logfile_filename="logfile.txt";

  ChUnitTestBaseBigFile()
  {
      log("Fixture constructor", false);
      less::Settings::autobuf = 0;
      less::Settings::logfile = default_logfile_fileid;
      less::Settings::follow_mode = 0;
      curr_ino = 0;
      curr_dev = 0;
      screen_trashed = TRASHED;
      less::Settings::sigs = 0;
      less::Settings::namelogfile = copyString(default_logfile_filename);
      lclIfile = new ifile::Ifile(nullptr);
  }

  void SetUp()
  {

      log("Setup start", false);

      EXPECT_CALL(GetMock<ChMock>(), 
        getCurrentIfile()).
        Times(2).
        WillOnce(Return(lclIfile)).
        WillOnce(Return(lclIfile));

      EXPECT_CALL(*lclIfile, 
        getFilestate()).
        Times(1).WillOnce(Return(nullptr));

      EXPECT_CALL(GetMock<ChMock>(), 
        lseek(default_file_id, 1, _)).
        Times(1).
        WillOnce(Return(1)); // Not BADLSEEK

      EXPECT_CALL(GetMock<ChMock>(), 
        lseek(default_file_id, 0, _)).
        Times(1).
        WillOnce(Return(1)); // Not BADLSEEK

      EXPECT_CALL(GetMock<ChMock>(), 
        filename::filesize(default_file_id)).
        Times(1).
        WillOnce(Return(default_fsize));

      EXPECT_CALL(*lclIfile, 
        setFilestate(_)).
        Times(1).
        WillOnce(DoAll(
          Invoke(this, &ChUnitTestBaseBigFile::captureFilestate), 
          Return()));

      init(default_file_id, 
              default_flags); 
      
      log("Setup end\n-------------------------------------------------------", false);
  }

  void TearDown()
  {
      log("TearDown", false);
      for (int i = 0; i < savedBufVector.size(); i++) {
        log("removing buff " + std::to_string(i));
        buf * b = savedBufVector[i];
        delete b;
      }
      
      if (thisfile->nbufs > 0) 
          ch_delbufs();

      free(less::Settings::namelogfile);
      
      if (! (capturedFilestate == nullptr))
          free(capturedFilestate);

      delete lclIfile;
      
  }
};
// --------------------------------------------------------------

TEST_F(ChUnitTestBaseBigFile, multi_block_get)
{
 
  createBufferForBlock(0);
  createBufferForBlock(9);
  createBufferForBlock(99);

  buf * block_0_buf = bufForBlock(0);
  buf * block_9_buf = bufForBlock(9);
  buf * block_99_buf = bufForBlock(99);

  // This contains all the expectations for all parts of this test
  EXPECT_CALL(GetMock<ChMock>(), 
     write(_, _, _)).//default_logfile_fileid, _, LBUFSIZE)).
     Times(3).
     WillRepeatedly(Return(LBUFSIZE));

  // ================================================================
  // Part 1 - Read 1st char in file - in block index 1
  // ================================================================
  currentBlock = 0;

  EXPECT_CALL(GetMock<ChMock>(), 
     iread(default_file_id, _, LBUFSIZE)).
     Times(1).
     WillOnce(DoAll(
            Invoke(this, &ChUnitTestBaseBigFile::captureAndUpdateBuffer), 
            Return(LBUFSIZE)));

  currentBlock = 0;
  
  int res = forw_get(); // part 1 
  
  char c = 0 + res;
  log ("forw_get res was char '" + std::string(1,c) + "'");

  EXPECT_EQ(thisfile->nbufs, 1);
  // did we get 1st char from buffer
  EXPECT_EQ(res, int(block_0_buf->data[0]));
  EXPECT_EQ(thisfile->offset, 1);


  // ================================================================
  // Part 2 - Seek to 1st char in block #9
  // ================================================================

  EXPECT_CALL(GetMock<ChMock>(), 
    lseek(default_file_id, _, _)).
    Times(1).
    WillOnce(Return(0)); 

  currentBlock = 9;
  
  res = seek(LBUFSIZE * 9); // part 2
  
  EXPECT_EQ(res, 0);

  EXPECT_CALL(GetMock<ChMock>(), 
     iread(default_file_id, _, LBUFSIZE)).
     Times(1).
     WillOnce(DoAll(
            Invoke(this, &ChUnitTestBaseBigFile::captureAndUpdateBuffer), 
            Return(LBUFSIZE)));

  // ================================================================
  // Part 3 = read 1st char in block #9
  // ================================================================
  
  res = forw_get();
  
  c = 0 + res;
  log ("forw_get res was char '" + std::string(1,c) + "'");

  EXPECT_EQ(thisfile->nbufs, 2);
  
  EXPECT_EQ(res, int(block_9_buf->data[0]));
  EXPECT_EQ(thisfile->offset, 1);
  EXPECT_EQ(tell(), (LBUFSIZE * 9) + 1);
  EXPECT_EQ(length(), default_fsize);

  // ================================================================
  // Part 4 = seek back to start of file again
  // ================================================================

  currentBlock = 0;
  
  res = beg_seek(); // part 4
  
  EXPECT_EQ(res, 0);

  // ================================================================
  // Part 5 = read 1st char in file again
  // ================================================================ 
  
  res = forw_get(); // part 5
  
  c = 0 + res;
  log ("forw_get res was char '" + std::string(1,c) + "'");

  EXPECT_EQ(thisfile->nbufs, 2);  
  EXPECT_EQ(res, int(block_0_buf->data[0]));
  EXPECT_EQ(thisfile->offset, 1);
  EXPECT_EQ(tell(), 1);

  // ================================================================
  // Part 6 - seek to the end of the file - block #99
  // ================================================================
    
  EXPECT_CALL(GetMock<ChMock>(), 
    lseek(default_file_id, _, _)).
    Times(1).
    WillOnce(Return(0)); 

  EXPECT_CALL(GetMock<ChMock>(), 
    filename::filesize(default_file_id)).
    Times(1).
    WillOnce(Return(default_fsize));

  currentBlock = 99;

  EXPECT_CALL(GetMock<ChMock>(), 
     iread(default_file_id, _, LBUFSIZE)).
     Times(1).
     WillOnce(DoAll(
            Invoke(this, &ChUnitTestBaseBigFile::captureAndUpdateBuffer), 
            Return(LBUFSIZE))); 
  
  res = end_seek(); // part 6
  
  EXPECT_EQ(res, 0);

  // ================================================================
  // Part 7 - Read last char in file - backwards
  // ================================================================
    
  res = back_get();
  
  c = 0 + res;
  log ("back_get res was char '" + std::string(1,c) + "'");

  EXPECT_EQ(thisfile->nbufs, 3);
  EXPECT_EQ(res, int(block_99_buf->data[LBUFSIZE-1]));
  EXPECT_EQ(thisfile->offset, LBUFSIZE-1);
  EXPECT_EQ(tell(), default_fsize-1);

  // ================================================================
  // Part 8 = seek back to start of file again
  // ================================================================

  currentBlock = 0;
  
  res = beg_seek(); 
  
  EXPECT_EQ(res, 0);

  // ================================================================
  // Part 9 = read back from start of file
  // ================================================================ 
  
  res = back_get(); 
  
  c = 0 + res;
  log ("part 9 back_get res was " + std::to_string(c) );

  EXPECT_EQ(thisfile->nbufs, 3);  
  EXPECT_EQ(res, -1);
  EXPECT_EQ(thisfile->offset, 0);
  EXPECT_EQ(tell(), 0);


}

TEST_F(ChUnitTestBaseBigFile, get_partial_block)
{
  createBufferForBlock(1);
  
  buf * block_1_buf = bufForBlock(1);
  
  // ================================================================
  // Part 1 - Seek to 1st char in 2nd block of file - block #1
  // ================================================================
  
  currentBlock = 1;

  int res = seek(LBUFSIZE); // part 1
  
  EXPECT_EQ(res, 0);

// TODO: Check buffered here? and in other places also?

  // ================================================================
  // Part 2 - Read 1st char in block #1, but dont allow entire block
  // to be read in - it will come in in multiple parts.
  // ================================================================

  EXPECT_CALL(GetMock<ChMock>(), 
    lseek(default_file_id, _, _)).
    Times(1).
    WillOnce(Return(0)); 

  EXPECT_CALL(GetMock<ChMock>(), 
     write(_, _, _)).
     Times(1).
     WillRepeatedly(Return(0));

  EXPECT_CALL(GetMock<ChMock>(), 
     iread(default_file_id, _, LBUFSIZE)).
     Times(1).
     WillOnce(DoAll(
            Invoke(this, &ChUnitTestBaseBigFile::captureAndUpdateBuffer), 
            Return(10))); // report only 10 chars of the buffer read


  res = forw_get(); // part 2
  
  char c = 0 + res;
  log ("pt2 forw_get res was char '" + std::string(1,c) + "'");

  EXPECT_EQ(thisfile->nbufs, 1);
  EXPECT_EQ(res, int(block_1_buf->data[0]));
  EXPECT_EQ(thisfile->offset, 1);
  EXPECT_EQ(tell(), LBUFSIZE+1);
  EXPECT_EQ(thisfile->fpos, LBUFSIZE+10);


  // ================================================================
  // Part 3 - seek in to buffer part that did not get read in part9
  // ================================================================
   
  res = seek(LBUFSIZE+11); // part 3
  EXPECT_EQ(res, 0);


  // ================================================================
  // Part 4 - read char from missing part of buffer
  // ================================================================
 
  res = seek(LBUFSIZE+11); // repeat
  EXPECT_EQ(res, 0);

  EXPECT_CALL(GetMock<ChMock>(), 
     write(_, _, _)).
     Times(1).
     WillRepeatedly(Return(0));

  EXPECT_CALL(GetMock<ChMock>(), 
     iread(default_file_id, _, LBUFSIZE-10)).
     Times(1).
     WillOnce(Return(LBUFSIZE-10)); 

  res = forw_get(); // part 4
  
  c = 0 + res;
  log ("pt4 forw_get res was char '" + std::string(1,c) + "'");

  EXPECT_EQ(thisfile->nbufs, 1);
  EXPECT_EQ(res, int(block_1_buf->data[11]));
  EXPECT_EQ(thisfile->offset, 11+1);
  EXPECT_EQ(tell(), LBUFSIZE+11+1);
  EXPECT_EQ(thisfile->fpos, 2*LBUFSIZE);

  // ================================================================
  // Part 5 - read last char in current buffer
  // ================================================================
  
  res = seek((LBUFSIZE*2) - 1);

  log ("pt5 end_buffer_seek res was " + std::to_string(res));

  res = forw_get(); 
  
  c = 0 + res;
  log ("pt5 forw_get res was char '" + std::string(1,c) + "'");

  EXPECT_EQ(thisfile->nbufs, 1);

  // ================================================================
  // Part 6 - unget char then read forward 
  // ================================================================
  
  EXPECT_EQ(ch_ungotchar, -1);

  const int ungot_char = res;

  ungetchar(res);

  EXPECT_EQ(ch_ungotchar, ungot_char);

  EXPECT_CALL(GetMock<ChMock>(), 
     write(_, _, 1)).
     Times(1).
     WillRepeatedly(Return(0));

  res = forw_get(); 

  c = 0 + res;
  log ("pt6 forw_get res was char '" + std::string(1,c) + "'");

  EXPECT_EQ(ch_ungotchar, -1);
  EXPECT_EQ(res, ungot_char);

}



// --------------------------------------------------------------
class ChUnitTestNonSeekableFile : public TestFixture {
public:

  std::exception invalid_setup_data;

  ifile::Ifile* lclIfile = nullptr;
  filestate* capturedFilestate = nullptr;
  unsigned char * capturedBuffer = nullptr;
  char bufferDataForTest[LBUFSIZE+1];

  std::map<int, buf *> savedBufMap; // block index
  std::vector<buf *> savedBufVector;

  int currentBlock = 0;
  bool skipCaptureAndUpdateBuffer = false;

  void captureFilestate(void* obj)
  {
      capturedFilestate = (filestate*)obj;
  }

  /*
   * createBufferForBlock - for the given block number create data to provide
   * in the test as data read from the file for that block
   */
  void createBufferForBlock(int blockNum) {

    std::string s;

    if (blockNum < 10)
      s = std::to_string(blockNum) + "BCDEFG";
    else if (blockNum < 100)
      s = std::to_string(blockNum) + "CDEFG";
    else if (blockNum < 1000) 
      s = std::to_string(blockNum) + "DEFG";
    else if (blockNum < 10000) 
      s = std::to_string(blockNum) + "EFG";

    //char default_data[7] = s.c_str();

    buf * currentBuf = new (buf);

    currentBuf->datasize = LBUFSIZE;

    memset(currentBuf->data, ' ', LBUFSIZE);
    currentBuf->data[LBUFSIZE-1] = '\0';

    strncpy(reinterpret_cast<char *>(currentBuf->data), s.c_str(), 7); 

    // Copy to the end of the buffer also
    
    char * bufPtr = reinterpret_cast<char *>(&currentBuf->data[LBUFSIZE-7]);
    strncpy(bufPtr, s.c_str(), 7);
    
    savedBufMap[blockNum] = currentBuf;
    savedBufVector.push_back(currentBuf);
  }

  /* 
   * bufForBlock - for the given block number return saved data 
   * for that block
   */
  buf * bufForBlock (int blockNum){
    auto res = savedBufMap.find(blockNum);
    if (res != savedBufMap.end()) 
      return res->second;
    else 
      throw invalid_setup_data;
  }


  /*
   * Capture for iread call
   * Capture the buffer in iread call and copy the saved
   * data for the current block to it
   */
  void captureAndUpdateBuffer(Unused, void* buffer, unsigned int len) {
        
    if (!skipCaptureAndUpdateBuffer) {
      capturedBuffer = (unsigned char *) buffer;
      
      buf * currentBuf = bufForBlock(currentBlock);
          
      strncpy(
        reinterpret_cast<char *>(capturedBuffer), 
        reinterpret_cast<char *>(currentBuf->data),
        len);

      log("Copied block " + std::to_string(currentBlock) +
        " to return buffer: >" + 
        std::string(reinterpret_cast<char *>(capturedBuffer)) +
        "<");
    }
  }


  /*
   * Capture for stat call
   * 
   */
  void setStat(Unused, struct stat * buf) {
    buf->st_ino=99;
  }


  // Assume file opened has file descriptor 1

  const int default_file_id = 1;
  const int default_flags = 0; // Note - not seekable
  const int default_fsize = 0; 
  const int default_fpos = 0;
  const int default_logfile_fileid = 2;
  const char * default_logfile_filename="logfile.txt";
  const char * ifile_filename = "test1.txt";

  ChUnitTestNonSeekableFile()
  {
    log("Fixture constructor", false);
    less::Settings::autobuf = 0;
    less::Settings::logfile = default_logfile_fileid;
    less::Settings::follow_mode = 0;
    curr_ino = 0;
    curr_dev = 0;
    screen_trashed = TRASHED;
    less::Settings::sigs = 0;
    less::Settings::namelogfile = copyString(default_logfile_filename);
    lclIfile = new ifile::Ifile(nullptr);
    
    less::Settings::ignore_eoi = 0;
    currentBlock = 0;
    less::Settings::follow_mode = FOLLOW_DESC;
  }

  void SetUp()
  {

      log("Setup start", false);

      EXPECT_CALL(GetMock<ChMock>(), 
        getCurrentIfile()).
        Times(2).
        WillOnce(Return(lclIfile)).
        WillOnce(Return(lclIfile));

      EXPECT_CALL(*lclIfile, 
        getFilestate()).
        Times(1).WillOnce(Return(nullptr));

      EXPECT_CALL(*lclIfile, 
        setFilestate(_)).
        Times(1).
        WillOnce(DoAll(
          Invoke(this, &ChUnitTestNonSeekableFile::captureFilestate), 
          Return()));

      init(default_file_id, 
              default_flags); 
      
      log("Setup end\n-------------------------------------------------------", false);
  }

  void TearDown()
  {
      log("TearDown", false);
      for (int i = 0; i < savedBufVector.size(); i++) {
        log("removing buff " + std::to_string(i));
        buf * b = savedBufVector[i];
        delete b;
      }
      savedBufMap.clear();
      savedBufVector.clear();
      
      if (thisfile->nbufs > 0) 
          ch_delbufs();

      free(less::Settings::namelogfile);
      
      if (! (capturedFilestate == nullptr))
          free(capturedFilestate);

      delete lclIfile;
      
  }
};
// --------------------------------------------------------------

TEST_F(ChUnitTestNonSeekableFile, read_from_pipe_file)
{
  createBufferForBlock(0);
  createBufferForBlock(1);
  
  buf * block_1_buf = bufForBlock(0);
  buf * block_2_buf = bufForBlock(1);

  currentBlock = 0;

  // ================================================================
  // Part 1 - read in 1st buffer from file
  // ================================================================
  
  EXPECT_CALL(GetMock<ChMock>(), 
     iread(default_file_id, _, LBUFSIZE)).
     Times(1).
     WillOnce(DoAll(
            Invoke(this, &ChUnitTestNonSeekableFile::captureAndUpdateBuffer), 
            Return(LBUFSIZE)));

  EXPECT_CALL(GetMock<ChMock>(), 
     write(_, _, _)).
     Times(1).
     WillRepeatedly(Return(0));


  int res = forw_get();

  char c = 0 + res;
  log ("pt1 forw_get res was char '" + std::string(1,c) + "'");

  // ================================================================
  // Part 2 - seek to end of buffer
  // ================================================================
  res = end_buffer_seek();
  
  EXPECT_EQ(res, 0); // success
  EXPECT_EQ(thisfile->block, 1);
  EXPECT_EQ(thisfile->offset, 0);

  // ================================================================
  // Part 3 - Set ignore eoi, set follow mode to follow_name and 
  //          do a ch_get 
  // ================================================================

  less::Settings::ignore_eoi = 1;
  currentBlock = 1;
  less::Settings::follow_mode = FOLLOW_NAME;

  EXPECT_CALL(GetMock<ChMock>(), 
     iread(default_file_id, _, LBUFSIZE)).
     Times(1).
     WillOnce(DoAll(
            Invoke(this, &ChUnitTestNonSeekableFile::captureAndUpdateBuffer), 
            Return(0)));

  EXPECT_CALL(GetMock<ChMock>(), 
     wait_message()).
     Times(1).
     WillRepeatedly(Return(nullptr));

  EXPECT_CALL(GetMock<ChMock>(), 
     ierror(_, _)).
     Times(1).
     WillRepeatedly(Return());

  EXPECT_CALL(GetMock<ChMock>(), 
    getCurrentIfile()).
    Times(1).
    WillOnce(Return(lclIfile));
  
  // This will capture the filestate object being saved
  EXPECT_CALL(*lclIfile, 
    getFilename()).
    Times(1).
    WillOnce(Return(nullptr));

  EXPECT_CALL(GetMock<ChMock>(), 
     stat(_, _)).
     Times(1).
     WillOnce(DoAll(
        Invoke(this, &ChUnitTestNonSeekableFile::setStat), 
        Return(0)));

  res = forw_get();

  c = 0 + res;
  log ("pt3 forw_get res was char '" + std::string(1,c) + "'");

  EXPECT_EQ(screen_trashed, TRASHED_AND_REOPEN_FILE);
  EXPECT_EQ(res, -1);
}


TEST_F(ChUnitTestNonSeekableFile, beg_seek)
{
  createBufferForBlock(2);
    
  buf * block_2_buf = bufForBlock(2);

  // ================================================================
  // Part 1 - read in starting at buffer 2
  // ================================================================

  currentBlock = 2;
  
  //EXPECT_CALL(GetMock<ChMock>(), 
  //  lseek(default_file_id, _, _)).
  //  Times(1).
  //  WillOnce(Return(0)); 
  
  // EXPECT_CALL(GetMock<ChMock>(), 
  //   iread(default_file_id, _, LBUFSIZE)).
  //   Times(1).
  //   WillOnce(DoAll(
  //     Invoke(this, &ChUnitTestNonSeekableFile::captureAndUpdateBuffer), 
  //     Return(8192)));
  //   //WillOnce(Return(8192));


  // EXPECT_CALL(GetMock<ChMock>(), 
  //    write(_, _, _)).
  //    Times(1).
  //    WillRepeatedly(Return(0));


  // log("start of test");
  // int res = seek(LBUFSIZE * 2); 

  // log("seek res = " + std::to_string(res));
  // //int res = beg_seek();
  
  // EXPECT_EQ(res, 1);


  // EXPECT_CALL(GetMock<ChMock>(), 
  //   iread(default_file_id, _, LBUFSIZE)).
  //   Times(2).
  //   WillOnce(Return(8192));

  // EXPECT_CALL(GetMock<ChMock>(), 
  //    write(_, _, _)).
  //    Times(1).
  //    WillRepeatedly(Return(0));


  // res = forw_get();

  // log ("pt1 forw_get res was char " + std::to_string(res));

  // EXPECT_EQ(res, -1);

  thisfile->fpos = 1;
  int res = beg_seek();
  log ("pt1 beg_seek res was " + std::to_string(res));



}


// TEST_F(ChUnitTestNonSeekableFile, helpfile)
// {
  
//   thisfile->flags = CH_HELPFILE;

//   createBufferForBlock(0);
    
//   buf * block_0_buf = bufForBlock(0);

//   // ================================================================
//   // Part 1 - read in 1st buffer from file
//   // ================================================================

//   currentBlock = 2;
  
//   EXPECT_CALL(GetMock<ChMock>(), 
//     lseek(default_file_id, _, _)).
//     Times(1).
//     WillOnce(Return(0)); 
    
//   int res = seek(LBUFSIZE * 2); 
//   //int res = beg_seek();
  
//   EXPECT_EQ(res, 1);

//    EXPECT_CALL(GetMock<ChMock>(), 
//      iread(default_file_id, _, LBUFSIZE)).
//      Times(1).
//      WillOnce(DoAll(
//             Invoke(this, &ChUnitTestNonSeekableFile::captureAndUpdateBuffer), 
//             Return(LBUFSIZE)));

//   EXPECT_CALL(GetMock<ChMock>(), 
//      write(_, _, _)).
//      Times(1).
//      WillRepeatedly(Return(0));

//   res = forw_get();

//   char c = 0 + res;
//   log ("pt1 forw_get res was char '" + std::string(1,c) + "'");

//   // ================================================================
//   // Part 4 - beg_seek, but seek issue 
//   // ================================================================   

// }


// tests done: Not testing all error conditions
// position_t tell(void);
// void init(int f, int flags);
// int seekable(int f);
// int getflags(void);
// void close(void);
// void set_eof(void);
// position_t length(void);
// void end_logfile(void);
// void sync_logfile(void);
// buffered
// int end_seek(void);
// tell
// flush
// ch_addbuf
// init_hashtbl
// int forw_get(void);
// int back_get(void);
// void setbufspace(int bufspace);
// length
// void ungetchar(int c);
// int seek(position_t pos);
// void flush(void);
// int end_buffer_seek(void);


// TODO:
// Add test to work on the helpfile


// int beg_seek(void);










// =================================================================
// Google Test can be run manually from the main() function
// or, it can be linked to the gtest_main library for an already
// set-up main() function primed to accept Google Test test cases.
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    // use this to run a filter of tests in the file
    //::testing::GTEST_FLAG(filter) = "ChUnitTestEmpty.ch_setbuffspace";
    //::testing::GTEST_FLAG(filter) = "ChUnitTestEmpty.ch_nullptr_tests";
    //::testing::GTEST_FLAG(filter) = "ChUnitTestNonSeekableFile.beg*";

    return RUN_ALL_TESTS();
}

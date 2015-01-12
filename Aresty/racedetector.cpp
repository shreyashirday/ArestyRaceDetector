#include<stdio.h>
#include<stdlib.h>
#include "pin.H"
#include <iostream>
#include <fstream>
#include <tr1/unordered_map>
#include <string>
#define PADSIZE 56


ofstream OutFile;
FILE * trace;
PIN_LOCK lock;
UINT32 cread = 0;
UINT32 cwrite = 0;
int c = 0;
INT32 numThreads = 0;
int dataRaces = 0;
int check = -1;
UINT32 id  = 0;
int slock = 0;
int sulock = 0;
int unq = 0;
int wunique = 0;
int numLocks = 0;
bool race = false;
VOID* raceLocation;

class thread_data_t
{
public:
  thread_data_t() : _countR(0), _countW(0) {}
  UINT64 _countR;
  UINT64 _countW;
   UINT8 _pad[PADSIZE];


};

class v_clock{
public:
 
  int* c;
};

class rw_clock{
public:
  int type;
  VOID* addr;
  int* c;
};

std::tr1::unordered_map<THREADID,v_clock*> threadhashtable;
std::tr1::unordered_map<VOID*,v_clock*> readhashtable;
std::tr1::unordered_map<VOID*,v_clock*> writehashtable;
std::tr1::unordered_map<VOID*,v_clock*> lockhashtable;



static TLS_KEY tls_key;


thread_data_t* get_tls(THREADID threadid)
{
  thread_data_t* tdata = static_cast<thread_data_t*>(PIN_GetThreadData(tls_key, threadid));
    return tdata;
}

v_clock* create_clock()
{
  v_clock* clock = new v_clock;
  
 
  clock->c = new int[3];
  clock->c[0]= 0;
  clock->c[1]=0;
  clock->c[2]=0;

  return clock;
}









//increase read count of thread by 1 and check if there is a read-write data race
VOID docountR(VOID*addr,THREADID threadid){
  thread_data_t* tdata = get_tls(threadid);
  tdata->_countR++;
  
  
  c++;
    
 
  std::tr1::unordered_map<VOID*,v_clock*>::const_iterator readgot = readhashtable.find(addr);
    std::tr1::unordered_map<THREADID,v_clock*>::const_iterator threadgot = threadhashtable.find(threadid);
    std::tr1::unordered_map<VOID*,v_clock*>::const_iterator writegot = writehashtable.find(addr);
    v_clock *clock = new v_clock;
    clock->c = new int[3];
    
    if(readgot==readhashtable.end()){
        clock->c[0] = threadgot->second->c[0];
        clock->c[1] = threadgot->second->c[1];
        clock->c[2] = threadgot->second->c[2];
        
        unq++;
        
        
        readhashtable.insert(std::make_pair<VOID*,v_clock*>(addr,clock));
      
    
  }
  else{
   
      if(writegot == writehashtable.end()){
        
          for(int i = 0; i < 3; i++){
              if(threadgot->second->c[i] > readgot->second->c[i]){
                  
              readgot->second->c[i] = threadgot->second->c[i];
                  
              }
          }
          
      }
      else{
          
          for(int i = 0; i < 3; i++){
              if(writegot->second->c[i] > threadgot->second->c[i]){
                  if(race==false){
                  dataRaces++;
                    race = true;
                      raceLocation = addr;
                  
                  break;
                  }
              }
          }
         
          for(int i = 0; i < 3;i++){
              if(threadgot->second->c[i] > readgot->second->c[i]){
                  readgot->second->c[i] = threadgot->second->c[i];
              }
              
          }
          
      }
  
  }


}



//increase write count of thread by 1 and checj if there is a write-write data race
VOID docountW(VOID*addr,THREADID threadid){
  thread_data_t* tdata = get_tls(threadid);
  tdata->_countW++;
  c++;
    

  std::tr1::unordered_map<VOID*,v_clock*>::const_iterator writegot = writehashtable.find(addr);
    std::tr1::unordered_map<THREADID,v_clock*>::const_iterator threadgot = threadhashtable.find(threadid);
    v_clock *clock = new v_clock;
    clock->c = new int[3];
  if(writegot==writehashtable.end()){
      clock->c[0] = threadgot->second->c[0];
      clock->c[1] = threadgot->second->c[1];
      clock->c[2] = threadgot->second->c[2];

      wunique++;
      
      writehashtable.insert(std::make_pair<VOID*,v_clock*>(addr,clock));
    
  }else{

      //address already has write clock created
      for(int i = 0; i < 3;i++){
          
          if(writegot->second->c[i] > threadgot->second->c[i]){
              //write-write data race
              if(race == false){
              dataRaces++;
              race = true;
                  raceLocation = addr;
    
              break;
              }
          }
          
      }
      
   
      for(int i = 0; i < 3;i++){
          if(threadgot->second->c[i] > writegot->second->c[i]){
              writegot->second->c[i] = threadgot->second->c[i];
          }
          
          
      }
      
  }

  
}


VOID CreateLockClock(VOID *ptr){
        v_clock *clock = create_clock();
    std::tr1::unordered_map<VOID*,v_clock*>::const_iterator lockgot = lockhashtable.find(ptr);
    if(lockgot == lockhashtable.end()){
        numLocks++;
        lockhashtable.insert(std::make_pair<VOID*,v_clock*>(ptr,clock));
    }
    
    
    
    
}


VOID AcquireLock(VOID*ptr,THREADID threadid){
 
  std::tr1::unordered_map<VOID*,v_clock*>::const_iterator lockgot = lockhashtable.find(ptr);
std::tr1::unordered_map<THREADID,v_clock*>::const_iterator threadgot = threadhashtable.find(threadid);

    slock++;
       if(lockgot != lockhashtable.end()){
 
        for(int i = 0; i < 3; i++){
            if(lockgot->second->c[i] > threadgot->second->c[i]){
                threadgot->second->c[i]  = lockgot->second->c[i];
            }
        }
    }
    
  
    
}


VOID ReleaseLock(VOID*ptr,THREADID threadid){
  
  std::tr1::unordered_map<THREADID,v_clock*>::const_iterator threadgot = threadhashtable.find(threadid);
  std::tr1::unordered_map<VOID*,v_clock*>::const_iterator lockgot = lockhashtable.find(ptr);
    
    sulock++;
    
    if(lockgot!=lockhashtable.end()){
        
        for(int i =0; i < 3; i++){
        if(threadgot->second->c[i] > lockgot->second->c[i]){
            lockgot->second->c[i] = threadgot->second->c[i];
            
        }
    }
       
        
    }
    else if(lockgot == lockhashtable.end()){
       
    }
    
    
        threadgot->second->c[threadid] +=1;

}





VOID ThreadStart(THREADID threadid, CONTEXT *ctx, INT32 flags, VOID *v){
  PIN_GetLock(&lock, threadid+1);
  
  numThreads++;
  v_clock *clock = create_clock();
   v_clock *mainclock = new v_clock;
    mainclock->c = new int[3];
    std::tr1::unordered_map<THREADID,v_clock*>::const_iterator got = threadhashtable.find(threadid);
 std::tr1::unordered_map<THREADID,v_clock*>::const_iterator gotagain =threadhashtable.find(0);
    if(got==threadhashtable.end()){
        
        if(threadid!=0){
           
            //vector clock of thread is max of main thread and <0,0,0>
            mainclock->c[0] = gotagain->second->c[0];
            mainclock->c[1] = gotagain->second->c[1];
            mainclock->c[2] = gotagain->second->c[2];
            
            mainclock->c[threadid]+=1;
            
        threadhashtable.insert(std::make_pair<THREADID,v_clock*>(threadid,mainclock));
            
            //update main thread vector clock
            gotagain->second->c[0]+=1;
   
            
        }else{
            //create main thread vector clock
     
            threadhashtable.insert(std::make_pair<THREADID,v_clock*>(threadid,clock));
            
        }
    }
    
    
  PIN_ReleaseLock(&lock);
  thread_data_t* tdata = new thread_data_t;
  PIN_SetThreadData(tls_key, tdata, threadid);
}
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v){
  PIN_GetLock(&lock, threadid+1);

  PIN_ReleaseLock(&lock);


}


VOID Trace(TRACE trace, VOID *v){

  for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl);bbl = BBL_Next(bbl)){

    for ( INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
      {
	UINT32 memOperands = INS_MemoryOperandCount(ins);

	for (UINT32 memOp = 0; memOp < memOperands; memOp++)
	  {
	    if (INS_MemoryOperandIsRead(ins, memOp))
	      {
INS_InsertPredicatedCall(
			 ins, IPOINT_BEFORE, (AFUNPTR)docountR,IARG_MEMORYOP_EA,memOp,IARG_THREAD_ID,
       				 IARG_END);


	      }
	else  if (INS_MemoryOperandIsWritten(ins, memOp))
	      {
		INS_InsertPredicatedCall(
					 ins, IPOINT_BEFORE, (AFUNPTR)docountW,IARG_MEMORYOP_EA,memOp,
					 IARG_THREAD_ID,
					 IARG_END);
	
	      }
	  }
      }
  }
}




// Is called for every instruction and instruments reads and writes


VOID ImageLoad(IMG img, VOID *){
    
    
  
  RTN rtn = RTN_FindByName(img, "_ShreyasAcquired");
  if( RTN_Valid(rtn)){
    RTN_Open(rtn);
      
       RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)CreateLockClock,IARG_FUNCARG_ENTRYPOINT_VALUE,0,IARG_END);
      RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)AcquireLock,IARG_FUNCARG_ENTRYPOINT_VALUE,0,IARG_THREAD_ID,IARG_END);
      RTN_Close(rtn);


  }
    
    RTN wRtn = RTN_FindByName(img,"_deposit");
    if(RTN_Valid(wRtn)){
        RTN_Open(wRtn);
        for(INS ins1 = RTN_InsHead(wRtn);INS_Valid(ins1); ins1 = INS_Next(ins1)){
            UINT32 mems = INS_MemoryOperandCount(ins1);
            for (UINT32 mem = 0; mem < mems; mem++){
                if(INS_MemoryOperandIsRead(ins1,mem)){
                    INS_InsertPredicatedCall(ins1,IPOINT_BEFORE,(AFUNPTR)docountR,IARG_MEMORYOP_EA,mem,IARG_THREAD_ID,IARG_END);
                }
                if(INS_MemoryOperandIsWritten(ins1,mem)){
                    INS_InsertPredicatedCall(ins1,IPOINT_BEFORE,(AFUNPTR)docountW,IARG_MEMORYOP_EA,mem,IARG_THREAD_ID,IARG_END);       
                }
            }         
        }
        RTN_Close(wRtn);        
    }
    
    RTN dRtn = RTN_FindByName(img,"_withdraw");
    if(RTN_Valid(dRtn)){
        RTN_Open(dRtn);
        for(INS ins1 = RTN_InsHead(dRtn);INS_Valid(ins1); ins1 = INS_Next(ins1)){
            UINT32 mems = INS_MemoryOperandCount(ins1);
            for (UINT32 mem = 0; mem < mems; mem++){
                if(INS_MemoryOperandIsRead(ins1,mem)){
                    INS_InsertPredicatedCall(ins1,IPOINT_BEFORE,(AFUNPTR)docountR,IARG_MEMORYOP_EA,mem,IARG_THREAD_ID,IARG_END);
                }
            if(INS_MemoryOperandIsWritten(ins1,mem)){
                    INS_InsertPredicatedCall(ins1,IPOINT_BEFORE,(AFUNPTR)docountW,IARG_MEMORYOP_EA,mem,IARG_THREAD_ID,IARG_END);
                }
            }
        }
        RTN_Close(dRtn);
    }
    
    RTN mRtn = RTN_FindByName(img,"_mainFunc");
    if(RTN_Valid(mRtn)){
        RTN_Open(mRtn);
        for(INS ins1 = RTN_InsHead(mRtn);INS_Valid(ins1); ins1 = INS_Next(ins1)){
            UINT32 mems = INS_MemoryOperandCount(ins1);
            for (UINT32 mem = 0; mem < mems; mem++){
                if(INS_MemoryOperandIsRead(ins1,mem)){
                    INS_InsertPredicatedCall(ins1,IPOINT_BEFORE,(AFUNPTR)docountR,IARG_MEMORYOP_EA,mem,IARG_THREAD_ID,IARG_END);
                }
                if(INS_MemoryOperandIsWritten(ins1,mem)){
                    INS_InsertPredicatedCall(ins1,IPOINT_BEFORE,(AFUNPTR)docountW,IARG_MEMORYOP_EA,mem,IARG_THREAD_ID,IARG_END);
                    
                }
            }
            
        }
        RTN_Close(mRtn);
    }
    
    RTN rtn1 = RTN_FindByName(img,"_ShreyasUnlock");
    if(RTN_Valid(rtn1)){
        RTN_Open(rtn1);
        
        RTN_InsertCall(rtn1,IPOINT_BEFORE,(AFUNPTR)ReleaseLock,IARG_FUNCARG_ENTRYPOINT_VALUE,0,IARG_THREAD_ID,IARG_END);
        
        
        
        
        RTN_Close(rtn1);
    }
    
    


}


KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "pinatrace.out", "specify output file name");

VOID Fini(INT32 code, VOID *v)
{
  
      if(dataRaces == 0){
        OutFile << "Race: NO" << endl;
    }
    else{
        OutFile << "Race: YES, at: " << raceLocation << endl;
    }
    
 
  
    OutFile << "done" << endl;

  OutFile.close();
    
  
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
    PIN_ERROR( "This Pintool finds data races in C++ programs\n"
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    PIN_InitLock(&lock);
    
    
    if (PIN_Init(argc, argv)) return Usage();
    PIN_InitSymbols();
    
     OutFile.open(KnobOutputFile.Value().c_str());
    //trace = fopen("pinatrace.out","w");

 
    
    tls_key = PIN_CreateThreadDataKey(0);
    
    
    
   
    PIN_AddThreadStartFunction(ThreadStart,0);
    
   
     IMG_AddInstrumentFunction(ImageLoad, 0);
    
    PIN_AddFiniFunction(Fini, 0);
    
    
    // Never returns
    PIN_StartProgram();
    
    return 0;
}

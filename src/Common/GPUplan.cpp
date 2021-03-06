#include "GPUplan.h"

#ifdef linux
#include "LinuxIO.h"
using namespace LinuxIO;
#endif

namespace Common
{

	void GPUNode::init()
	{
		for (int i = 0; i < nMaxEvents; i++)
			gpuSafeCall( cudaEventCreate(&event[i]), "GPUNode::init(): on event create" ); 
		gpuSafeCall( cudaStreamCreate(&stream), "GPUNode::init(): on stream create" );
		gpuSafeCall( cudaStreamCreate(&stream2), "GPUNode::init(): on stream create" );
	}

	void GPUNode::destroy()
	{
		for (int i = 0; i < nMaxEvents; i++)
			gpuSafeCall( cudaEventDestroy(event[i]), "GPUNode::~GPUNode(): on event destroy" );
		gpuSafeCall( cudaStreamDestroy(stream), "GPUNode::~GPUNode(): on stream destroy" );
		gpuSafeCall( cudaStreamDestroy(stream2), "GPUNode::~GPUNode(): on stream destroy" );
	}

	bool GPUplan::isInstance = false;
	GPUplan *GPUplan::self = NULL;

	GPUplan::GPUplan()
	{
		nGPU = nMaxGPU = 0; plan = NULL;  data1D = 0; ibegin = 0;
	}

  void GPUplan::init()
	{
#if MGPU_EMU
		nMaxGPU = nGPU = GPU_NUM;
#else
		gpuSafeCall( cudaGetDeviceCount(&nMaxGPU),  "GPUplan::init(): cudaGetDeviceCount" );
#endif
		if (nMaxGPU < 1)
			throw std::runtime_error("GPUplan::init(): no CUDA capable devices\n"); 
		nGPU  = nMaxGPU;
		plan = new GPUNode*[nMaxGPU];
		for ( int i = ibegin; i < ibegin + nMaxGPU; i++)
		{
			gpuSafeCall(cudaSetDevice(i), "GPUplan::init(): cudaSetDevice", i);
			plan[i - ibegin] = new GPUNode();
			plan[i - ibegin]->init();	

#if !MGPU_EMU
			int canAccessPeer;
			cudaError_t status;
			if (i < nMaxGPU - 1)
			{
				cudaDeviceCanAccessPeer(&canAccessPeer, i, i+1);
				if (canAccessPeer == 1)
				{
					status = cudaDeviceEnablePeerAccess(i+1, 0);
					if (status == cudaSuccess || status ==  cudaErrorPeerAccessAlreadyEnabled)
						printf("Enabling peer access from device %d on device %d\n", i, i+1);
				}
			}
			if (i > 0)
			{
				cudaDeviceCanAccessPeer(&canAccessPeer, i, i-1);
				if (canAccessPeer == 1)
				{
					status = cudaDeviceEnablePeerAccess(i-1, 0);
					if (status == cudaSuccess || status ==  cudaErrorPeerAccessAlreadyEnabled)
						printf("Enabling peer access from device %d on device %d\n", i, i-1);
				}
			}
#endif
		}
	}

	GPUplan* GPUplan::Instance()
	{
		if(!isInstance)
		{
			self = new GPUplan();
			isInstance = true;
			return self;
		}
		else
			return self;
	}

	GPUNode* GPUplan::node(int i)
	{
		if (plan == NULL || i >= nGPU)
			throw std::logic_error("GPUplan::node: Indexing");
		return plan[i];
	}	

	void GPUplan::setDevice(int iDev)
	{
		gpuSafeCall(cudaSetDevice(ibegin + iDev), "GPUplan::setDevice", iDev);
	}

	void GPUplan::deviceSynchronize()
	{
		for (int i = 0; i < nGPU; i++)
		{
			gpuSafeCall(cudaSetDevice(i + ibegin), "GPUplan::deviceSynchronize(): cudaSetDevice", i);
			gpuSafeCall(cudaDeviceSynchronize(), "GPUplan::deviceSynchronize(): cudaDeviceSynchronize", i);
		}
	}

	void GPUplan::setBegin(int iDev)
	{
		ibegin = iDev;
	}

	void GPUplan::setGPUnum( int num )
	{
		nGPU = (num <= nMaxGPU)?num:nMaxGPU; 
	}

	void GPUplan::splitEven1D(int num_elems_1D)
	/*
		Splits num_elems_total along num_elems_1D into GPU_N chunks
		of size num_elems_total/GPU_N taking into account
		data sizes nondivisible by GPU count
	*/
	{
    //Subdividing input data across GPUs
    //Get data sizes for each GPU
		data1D = num_elems_1D;
		for (int iDev = 0; iDev < nGPU; iDev++)
			plan[iDev]->setLength1D(num_elems_1D / (int)nGPU);
    //Take into account "odd" data sizes
    for(int iDev = 0; iDev < num_elems_1D % (int)nGPU; iDev++)
			plan[iDev]->setLength1D(plan[iDev]->getLength1D()+1);
		printf("Splitting the data along first dimension(%d):\n", num_elems_1D);
		for(int iDev = 0; iDev < nGPU; iDev++)
			printf("device(%d) = %d    ",ibegin + iDev, plan[iDev]->getLength1D());
		printf("\n");
	}

	void GPUplan::split1D(int *num_elems_1D)
	{
		data1D = 0;
		for (int i = 0; i < nGPU; i++)
		{
			data1D += num_elems_1D[i];
			plan[i]->setLength1D(num_elems_1D[i]);
		}
		printf("Splitting the data along first dimension(%d):\n", data1D);
		for(int i = 0; i < nGPU; i++)
			printf("device(%d) = %d    ",ibegin + i, plan[i]->getLength1D());
		printf("\n");
	}

	void GPUplan::destroy()
	{
		isInstance = false;
		for (int i = 0; i < nMaxGPU; i++)
		{
			cudaSetDevice(i + ibegin);
			plan[i]->destroy();
		}
		delete [] plan;
	}

	GPUplan::~GPUplan()
	{
		GPUplan::destroy();
	}

	void gpuSafeCall(cudaError status, char* message, int id, char *file, int linenum)
	{
		if (status != cudaSuccess )
		{
			int len = strlen(message);
			len += 16;
			if (id < 0)
				cudaGetDevice(&id);
			else
			{
				GPUplan *pGPUplan = GPUplan::Instance();
				id += pGPUplan->begin();
			}
			const int bufSize = 256;
			if (len > bufSize)
				throw (std::logic_error("gpuSafeCall: buffer size is too small"));
			char buffer[bufSize];
			sprintf_s(buffer, "%s on device %d: %d %s\nEncountered in %s, line# %d", message, id, int(status), cudaGetErrorString(status), file, linenum);
			throw std::runtime_error(buffer);
		}
	}

	GPUplan *CheckGPUplan(int num_elems_total)
	{
		GPUplan *pGPUplan = GPUplan::Instance();
		if (num_elems_total % pGPUplan->getLength1D())
			throw std::runtime_error("CheckGPUplan: num_elems_total should be divisible by dimx!\n"); 		
		return pGPUplan;
	}
}                   

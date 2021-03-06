CUDA_INSTALL_PATH = $(CUDA_HOME)
#NETCDF_INSTALL_PATH = /usr
NETCDF_INSTALL_PATH = /home-2/nmarkovskiy/local

CXX := mpicc -fopenmp 
LINK := mpicc -fPIC -fopenmp 
NVCC := nvcc -arch=compute_20 -code=sm_20
# -ccbin /usr/bin

# Includes
INCLUDES = -I. -I$(CUDA_INSTALL_PATH)/include -I$(NETCDF_INSTALL_PATH)/include -I../FluidSolver2D -I$(MPIHOME)/include

# Common flags
COMMONFLAGS += $(INCLUDES) -O0
NVCCFLAGS += $(COMMONFLAGS) 
#-g -G
CXXFLAGS += $(COMMONFLAGS) 
#-g
CFLAGS += $(COMMONFLAGS)

# Libs
LIB_CUDA :=  -L$(CUDA_INSTALL_PATH)/lib64 -lcudart
LIB_NETCDF := -L$(NETCDF_INSTALL_PATH)/lib -L/opt/hdf5/serial/lib -lnetcdf  -lnetcdff -lhdf5_hl -lhdf5 -lgfortran 
LIB_MPI := -L$(MPIHOME)/lib

OBJS = AdiSolver3D.cpp.o AdiSolver3D.cu.o ../FluidSolver2D/Grid2D.cpp.o Grid3D.cpp.o  \
       TimeLayer3D.cu.o Solver3D.cpp.o FluidSolver3D.cpp.o LinuxIO.cpp.o GPUplan.cpp.o PARAplan.cpp.o test_util.cpp.o
TARGET = ../../bin/Release/FluidSolver3D
LINKLINE = $(LINK) -o $(TARGET) $(OBJS) $(LIB_CUDA) $(LIB_NETCDF) $(LIB_MPI) -lrt 

.SUFFIXES: .cpp .cu .o

all: $(OBJS) Makefile
	$(LINKLINE)

%.cu.o: %.cu
	$(NVCC) -c $(NVCCFLAGS) $< -o $@

%.cpp.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

../FluidSolver2D/Grid2D.cpp.o: ../FluidSolver2D/Grid2D.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

LinuxIO.cpp.o: ../Common/LinuxIO.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
GPUplan.cpp.o: ../Common/GPUplan.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
PARAplan.cpp.o: ../Common/PARAplan.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@	

test_util.cpp.o: ../Common/test_util.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@	

clean:
	rm $(TARGET) *.o ../FluidSolver2D/*.o

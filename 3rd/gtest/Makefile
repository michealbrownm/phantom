GTEST = gtest.a
GTEST_PATH = ./

CPPFLAGS += -isystem $(GTEST_PATH)/include
CXXFLAGS += -g -Wall -Wextra -pthread

GTEST_HEADERS = $(GTEST_PATH)/include/gtest/*.h \
		$(GTEST_PATH)/include/gtest/internal/*.h



all : $(GTEST)

clean : 
	rm -rf gtest-all.o && rm -rf gtest_main.o && rm -rf gtest.a

GTEST_SRCS = $(GTEST_PATH)/src/*.cc $(GTEST_PATH)/src/*.h $(GTEST_HEADERS) 

gtest-all.o : $(GTEST_SRCS)
	$(CXX) $(CPPFLAGS) -I$(GTEST_PATH) $(CXXFLAGS) -c \
	$(GTEST_PATH)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS)
	$(CXX) $(CPPFLAGS) -I$(GTEST_PATH) $(CXXFLAGS) -c \
		$(GTEST_PATH)/src/gtest_main.cc

$(GTEST) : gtest-all.o 
	$(AR) $(ARFLAGS) $@ $^

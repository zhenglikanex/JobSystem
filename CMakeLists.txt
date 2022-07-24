cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)
project(JobSystem)

add_executable(JobSystem
	src/work_stealing_queue.hpp
	src/work_stealing_queue.cpp
	src/job.hpp
	src/job.cpp
	src/job_system.hpp
	src/job_system.cpp
	src/main.cpp
)
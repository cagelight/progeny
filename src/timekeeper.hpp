#pragma once

#include <ctime>


struct timeunit : public timespec {
	timeunit() : timespec {0, 0} {}
	timeunit(double);
	operator double();
	timeunit operator + (timeunit const & other);
	timeunit & operator += (timeunit const & other);
	timeunit operator - (timeunit const & other);
	timeunit operator * (double mult);
};

class timekeeper {
	
public:
	
	enum struct clock_type : clockid_t {
		monotonic = CLOCK_MONOTONIC,
		process_exec = CLOCK_PROCESS_CPUTIME_ID,
		thread_exec = CLOCK_THREAD_CPUTIME_ID,
	};
	
	timekeeper(clock_type);
	
	double const & impulse {impulse_};
	uint32_t const & elapsed_msec {elapsed_msec_};
	double const & timescale {timescale_};
	
	void mark();
	void mark_and_change_timescale(double);
	void reset();
	void sleep_for_target(double target_fps);
	timeunit phantom_mark();
	
private:
	
	clockid_t llct;
	timeunit ts_accum;
	timeunit ts_mark;
	
	double impulse_ = 0.0D;
	uint32_t elapsed_msec_ = 0;
	double timescale_ = 1.0D;
};

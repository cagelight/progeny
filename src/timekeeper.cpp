#include "com.hpp"
#include "timekeeper.hpp"

#include <ctgmath>

timeunit::timeunit(double v) {
	double f = floor(v);
	this->tv_sec = f;
	this->tv_nsec = (v - f) * 1000000000;
}

timeunit::operator double() {
	return static_cast<double>(this->tv_sec) + static_cast<double>(this->tv_nsec) / 1000000000;
}

timeunit timeunit::operator + (timeunit const & other) {
	timeunit res;
	res.tv_sec = this->tv_sec + other.tv_sec;
	res.tv_nsec = this->tv_nsec + other.tv_nsec;
	long m = res.tv_nsec / 1000000000;
	if (res.tv_nsec < 0) {
		res.tv_nsec += (m - 1) * -1000000000;
		res.tv_sec -= -(m - 1);
	} else {
		res.tv_nsec -= m * 1000000000;
		res.tv_sec += m;
	}
	return res;
}

timeunit & timeunit::operator += (timeunit const & other) {
	this->tv_sec += other.tv_sec;
	this->tv_nsec += other.tv_nsec;
	long m = this->tv_nsec / 1000000000;
	if (this->tv_nsec < 0) {
		this->tv_nsec += (m - 1) * -1000000000;
		this->tv_sec -= -(m - 1);
	} else {
		this->tv_nsec -= m * 1000000000;
		this->tv_sec += m;
	}
	return *this;
}

timeunit timeunit::operator - (timeunit const & other) {
	timeunit res;
	res.tv_sec = this->tv_sec - other.tv_sec;
	res.tv_nsec = this->tv_nsec - other.tv_nsec;
	long m = res.tv_nsec / 1000000000;
	if (res.tv_nsec < 0) {
		res.tv_nsec += (m - 1) * -1000000000;
		res.tv_sec -= -(m - 1);
	} else {
		res.tv_nsec -= m * 1000000000;
		res.tv_sec += m;
	}
	return res;
}

timeunit timeunit::operator * (double mult) {
	timeunit res = *this;
	res.tv_nsec *= mult;
	res.tv_sec *= mult;
	long m = res.tv_nsec / 1000000000;
	if (res.tv_nsec < 0) {
		res.tv_nsec += (m - 1) * -1000000000;
		res.tv_sec -= -(m - 1);
	} else {
		res.tv_nsec -= m * 1000000000;
		res.tv_sec += m;
	}
	return res;
}

timekeeper::timekeeper(clock_type ct) : llct(static_cast<clockid_t>(ct)) {
	clock_gettime(llct, &ts_mark);
}

void timekeeper::mark() {
	timeunit nmark;
	clock_gettime(llct, &nmark);
	timeunit since {(nmark - ts_mark) * timescale};
	impulse_ = since.tv_sec;
	impulse_ += static_cast<double>(since.tv_nsec) / 1000000000;
	ts_accum += since;
	elapsed_msec_ = ts_accum.tv_sec * 1000;
	elapsed_msec_ += ts_accum.tv_nsec / 1000000;
	ts_mark = nmark;
}

void timekeeper::mark_and_change_timescale(double t) {
	mark();
	timescale_ = t;
}

void timekeeper::reset() {
	clock_gettime(llct, &ts_mark);
	ts_accum = {};
	elapsed_msec_ = 0;
	impulse_ = 0;
}

void timekeeper::sleep_for_target(double target) {
	timeunit dest {1 / target};
	timeunit slp = dest - phantom_mark();
	if (dest.tv_sec >= 0) nanosleep(&slp, nullptr);
}

timeunit timekeeper::phantom_mark() {
	timeunit nmark;
	clock_gettime(llct, &nmark);
	return (nmark - ts_mark) * timescale;
}

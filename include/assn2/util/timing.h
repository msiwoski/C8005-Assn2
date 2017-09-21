//
// Created by shane on 2/19/17.
//

#ifndef COMP8005_ASSN2_TIMING_H
#define COMP8005_ASSN2_TIMING_H

/**
 * Gets the difference between a start and an end struct timeval. The returned value should fit into a time_t (probably).
 */
#define TIME_DIFF(start, end) (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);

#endif //COMP8005_ASSN2_TIMING_H

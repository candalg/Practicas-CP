#ifndef __OPTIONS_H__
#define __OPTIONS_H__

struct options {
	int barbers;
	int customers;
	int cut_time;
};

int read_options(int argc, char **argv, struct options *opt);

#endif
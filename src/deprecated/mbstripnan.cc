/*--------------------------------------------------------------------
 *    The MB-system:    mbstripNaN.c        8/8/02
 *
 *    Copyright (c) 2002-2020 by
 *    Mike McCann (mccann@mbari.org)
 *    David W. Caress (caress@mbari.org)
 *      Monterey Bay Aquarium Research Institute
 *      Moss Landing, CA 95039
 *    and Dale N. Chayes (dale@ldeo.columbia.edu)
 *      Lamont-Doherty Earth Observatory
 *      Palisades, NY 10964
 *
 *    See README file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/

/* mbstripnan.c
 * Filter output from GMT's grd2xyz removing any nodes
 * that have NaN elevations.  Output meant to feed into GMT's surface
 * in support of the mbm_grd2geovrml macro.
 *
 * Mike McCann   8 August 2002
 * MBARI
 */

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

struct node {
	double lon;
	double lat;
	double height;
};

/*
 * Read double x,y,z on stdin and send to stdout all but NaNs
 */
int main() {
	struct node n;
        const size_t chunk = sizeof(n);
	while ((fread(&n, chunk, 1, stdin) != chunk)) {
		if (std::isnan(n.lon) || std::isnan(n.lat) || std::isnan(n.height)) {
			continue;
		}
		fwrite(&n, chunk, 1, stdout);
	}
	exit(EXIT_SUCCESS);
}

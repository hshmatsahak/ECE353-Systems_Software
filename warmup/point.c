#include <assert.h>
#include "common.h"
#include "point.h"
#include <math.h>

void
point_translate(struct point *p, double x, double y)
{
	p->x += x;
	p->y += y;
}

double
point_distance(const struct point *p1, const struct point *p2)
{
	double distance = sqrt(pow(p1->x-p2->x, 2) + pow(p1->y-p2->y, 2));
    return distance;
}

int
point_compare(const struct point *p1, const struct point *p2)
{
	double p1_dist = sqrt(pow(p1->x, 2) + pow(p1->y, 2));
	double p2_dist = sqrt(pow(p2->x, 2) + pow(p2->y, 2));
	double diff = p1_dist - p2_dist;
	if (diff<0)
		return -1;
	else if(diff>0)
		return 1;
	return 0;
}

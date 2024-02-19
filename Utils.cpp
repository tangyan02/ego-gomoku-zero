//
// Created by 唐雁 on 2024/2/19.
//

#include "Utils.h"

long long getSystemTime() {
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
}
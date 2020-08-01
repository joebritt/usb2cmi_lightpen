
// This table was generated with an excel spreadsheet.
//
// The clock used to drive the Output Compare pulse generator runs at
// 80MHz.  The Series I/II CMI dotclock is 10.65MHz.
//
// Note that the Q045 graphics card has 2 pots (RV1 and RV2) that 
// control one-shots which determine the active area location
// and width.
// Because of that, it is not safe for us to assume hard values
// for the bias offset and right edge of the screen.
// For Q045 systems, we allow the cursor to run off the screen on either side.
// We can safely pin the top and bottom positions, however.

unsigned int pixel_offsets_II[512] = { 
900,
908,
916,
923,
931,
938,
946,
953,
961,
968,
976,
983,
991,
998,
1006,
1013,
1021,
1028,
1036,
1043,
1051,
1058,
1066,
1073,
1081,
1088,
1096,
1103,
1111,
1118,
1126,
1133,
1141,
1148,
1156,
1163,
1171,
1178,
1186,
1193,
1201,
1208,
1216,
1224,
1231,
1239,
1246,
1254,
1261,
1269,
1276,
1284,
1291,
1299,
1306,
1314,
1321,
1329,
1336,
1344,
1351,
1359,
1366,
1374,
1381,
1389,
1396,
1404,
1411,
1419,
1426,
1434,
1441,
1449,
1456,
1464,
1471,
1479,
1486,
1494,
1501,
1509,
1516,
1524,
1531,
1539,
1547,
1554,
1562,
1569,
1577,
1584,
1592,
1599,
1607,
1614,
1622,
1629,
1637,
1644,
1652,
1659,
1667,
1674,
1682,
1689,
1697,
1704,
1712,
1719,
1727,
1734,
1742,
1749,
1757,
1764,
1772,
1779,
1787,
1794,
1802,
1809,
1817,
1824,
1832,
1839,
1847,
1854,
1862,
1870,
1877,
1885,
1892,
1900,
1907,
1915,
1922,
1930,
1937,
1945,
1952,
1960,
1967,
1975,
1982,
1990,
1997,
2005,
2012,
2020,
2027,
2035,
2042,
2050,
2057,
2065,
2072,
2080,
2087,
2095,
2102,
2110,
2117,
2125,
2132,
2140,
2147,
2155,
2162,
2170,
2177,
2185,
2193,
2200,
2208,
2215,
2223,
2230,
2238,
2245,
2253,
2260,
2268,
2275,
2283,
2290,
2298,
2305,
2313,
2320,
2328,
2335,
2343,
2350,
2358,
2365,
2373,
2380,
2388,
2395,
2403,
2410,
2418,
2425,
2433,
2440,
2448,
2455,
2463,
2470,
2478,
2485,
2493,
2500,
2508,
2516,
2523,
2531,
2538,
2546,
2553,
2561,
2568,
2576,
2583,
2591,
2598,
2606,
2613,
2621,
2628,
2636,
2643,
2651,
2658,
2666,
2673,
2681,
2688,
2696,
2703,
2711,
2718,
2726,
2733,
2741,
2748,
2756,
2763,
2771,
2778,
2786,
2793,
2801,
2808,
2816,
2824,
2831,
2839,
2846,
2854,
2861,
2869,
2876,
2884,
2891,
2899,
2906,
2914,
2921,
2929,
2936,
2944,
2951,
2959,
2966,
2974,
2981,
2989,
2996,
3004,
3011,
3019,
3026,
3034,
3041,
3049,
3056,
3064,
3071,
3079,
3086,
3094,
3101,
3109,
3116,
3124,
3131,
3139,
3147,
3154,
3162,
3169,
3177,
3184,
3192,
3199,
3207,
3214,
3222,
3229,
3237,
3244,
3252,
3259,
3267,
3274,
3282,
3289,
3297,
3304,
3312,
3319,
3327,
3334,
3342,
3349,
3357,
3364,
3372,
3379,
3387,
3394,
3402,
3409,
3417,
3424,
3432,
3439,
3447,
3454,
3462,
3470,
3477,
3485,
3492,
3500,
3507,
3515,
3522,
3530,
3537,
3545,
3552,
3560,
3567,
3575,
3582,
3590,
3597,
3605,
3612,
3620,
3627,
3635,
3642,
3650,
3657,
3665,
3672,
3680,
3687,
3695,
3702,
3710,
3717,
3725,
3732,
3740,
3747,
3755,
3762,
3770,
3777,
3785,
3793,
3800,
3808,
3815,
3823,
3830,
3838,
3845,
3853,
3860,
3868,
3875,
3883,
3890,
3898,
3905,
3913,
3920,
3928,
3935,
3943,
3950,
3958,
3965,
3973,
3980,
3988,
3995,
4003,
4010,
4018,
4025,
4033,
4040,
4048,
4055,
4063,
4070,
4078,
4085,
4093,
4100,
4108,
4116,
4123,
4131,
4138,
4146,
4153,
4161,
4168,
4176,
4183,
4191,
4198,
4206,
4213,
4221,
4228,
4236,
4243,
4251,
4258,
4266,
4273,
4281,
4288,
4296,
4303,
4311,
4318,
4326,
4333,
4341,
4348,
4356,
4363,
4371,
4378,
4386,
4393,
4401,
4408,
4416,
4424,
4431,
4439,
4446,
4454,
4461,
4469,
4476,
4484,
4491,
4499,
4506,
4514,
4521,
4529,
4536,
4544,
4551,
4559,
4566,
4574,
4581,
4589,
4596,
4604,
4611,
4619,
4626,
4634,
4641,
4649,
4656,
4664,
4671,
4679,
4686,
4694,
4701,
4709,
4716,
4724,
4731,
4739
};



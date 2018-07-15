#include <Arduino.h>

q15 _fm_sine[] = {
0,50,100,150,201,251,301,351,
402,452,502,552,603,653,703,753,
803,854,904,954,1004,1054,1104,1155,
1205,1255,1305,1355,1405,1455,1505,1555,
1605,1655,1705,1755,1805,1855,1905,1955,
2005,2055,2105,2155,2204,2254,2304,2354,
2403,2453,2503,2553,2602,2652,2701,2751,
2800,2850,2899,2949,2998,3048,3097,3146,
3196,3245,3294,3344,3393,3442,3491,3540,
3589,3638,3687,3736,3785,3834,3883,3932,
3980,4029,4078,4126,4175,4224,4272,4321,
4369,4418,4466,4514,4563,4611,4659,4707,
4755,4803,4851,4899,4947,4995,5043,5091,
5139,5186,5234,5282,5329,5377,5424,5472,
5519,5566,5613,5661,5708,5755,5802,5849,
5896,5943,5990,6036,6083,6130,6176,6223,
6269,6316,6362,6408,6454,6501,6547,6593,
6639,6685,6731,6776,6822,6868,6913,6959,
7004,7050,7095,7140,7186,7231,7276,7321,
7366,7411,7455,7500,7545,7589,7634,7678,
7723,7767,7811,7855,7899,7943,7987,8031,
8075,8119,8162,8206,8249,8293,8336,8379,
8422,8465,8508,8551,8594,8637,8680,8722,
8765,8807,8849,8892,8934,8976,9018,9060,
9102,9143,9185,9227,9268,9310,9351,9392,
9433,9474,9515,9556,9597,9638,9678,9719,
9759,9799,9840,9880,9920,9960,10000,10039,
10079,10119,10158,10198,10237,10276,10315,10354,
10393,10432,10471,10509,10548,10586,10624,10663,
10701,10739,10777,10814,10852,10890,10927,10965,
11002,11039,11076,11113,11150,11187,11224,11260,
11297,11333,11369,11405,11441,11477,11513,11549,
11584,11620,11655,11691,11726,11761,11796,11830,
11865,11900,11934,11969,12003,12037,12071,12105,
12139,12173,12206,12240,12273,12306,12339,12372,
12405,12438,12471,12503,12536,12568,12600,12632,
12664,12696,12728,12759,12791,12822,12853,12884,
12915,12946,12977,13008,13038,13068,13099,13129,
13159,13189,13218,13248,13278,13307,13336,13365,
13394,13423,13452,13481,13509,13538,13566,13594,
13622,13650,13677,13705,13733,13760,13787,13814,
13841,13868,13895,13921,13948,13974,14000,14026,
14052,14078,14104,14129,14154,14180,14205,14230,
14255,14279,14304,14328,14353,14377,14401,14425,
14448,14472,14496,14519,14542,14565,14588,14611,
14634,14656,14679,14701,14723,14745,14767,14788,
14810,14831,14853,14874,14895,14916,14936,14957,
14977,14998,15018,15038,15058,15078,15097,15117,
15136,15155,15174,15193,15212,15230,15249,15267,
15285,15303,15321,15339,15356,15374,15391,15408,
15425,15442,15459,15475,15492,15508,15524,15540,
15556,15572,15587,15603,15618,15633,15648,15663,
15678,15692,15706,15721,15735,15749,15762,15776,
15790,15803,15816,15829,15842,15855,15867,15880,
15892,15904,15916,15928,15940,15951,15963,15974,
15985,15996,16007,16017,16028,16038,16048,16058,
16068,16078,16088,16097,16106,16115,16124,16133,
16142,16150,16159,16167,16175,16183,16191,16198,
16206,16213,16220,16227,16234,16241,16247,16254,
16260,16266,16272,16278,16283,16289,16294,16299,
16304,16309,16314,16318,16323,16327,16331,16335,
16339,16342,16346,16349,16352,16355,16358,16361,
16363,16366,16368,16370,16372,16374,16375,16377,
16378,16379,16380,16381,16382,16382,16383,16383,
16383,16383,16383,16382,16382,16381,16380,16379,
16378,16377,16375,16374,16372,16370,16368,16366,
16363,16361,16358,16355,16352,16349,16346,16342,
16339,16335,16331,16327,16323,16318,16314,16309,
16304,16299,16294,16289,16283,16278,16272,16266,
16260,16254,16247,16241,16234,16227,16220,16213,
16206,16198,16191,16183,16175,16167,16159,16150,
16142,16133,16124,16115,16106,16097,16088,16078,
16068,16058,16048,16038,16028,16017,16007,15996,
15985,15974,15963,15951,15940,15928,15916,15904,
15892,15880,15867,15855,15842,15829,15816,15803,
15790,15776,15762,15749,15735,15721,15706,15692,
15678,15663,15648,15633,15618,15603,15587,15572,
15556,15540,15524,15508,15492,15475,15459,15442,
15425,15408,15391,15374,15356,15339,15321,15303,
15285,15267,15249,15230,15212,15193,15174,15155,
15136,15117,15097,15078,15058,15038,15018,14998,
14977,14957,14936,14916,14895,14874,14853,14831,
14810,14788,14767,14745,14723,14701,14679,14656,
14634,14611,14588,14565,14542,14519,14496,14472,
14448,14425,14401,14377,14353,14328,14304,14279,
14255,14230,14205,14180,14154,14129,14104,14078,
14052,14026,14000,13974,13948,13921,13895,13868,
13841,13814,13787,13760,13733,13705,13677,13650,
13622,13594,13566,13538,13509,13481,13452,13423,
13394,13365,13336,13307,13278,13248,13218,13189,
13159,13129,13099,13068,13038,13008,12977,12946,
12915,12884,12853,12822,12791,12759,12728,12696,
12664,12632,12600,12568,12536,12503,12471,12438,
12405,12372,12339,12306,12273,12240,12206,12173,
12139,12105,12071,12037,12003,11969,11934,11900,
11865,11830,11796,11761,11726,11691,11655,11620,
11584,11549,11513,11477,11441,11405,11369,11333,
11297,11260,11224,11187,11150,11113,11076,11039,
11002,10965,10927,10890,10852,10814,10777,10739,
10701,10663,10624,10586,10548,10509,10471,10432,
10393,10354,10315,10276,10237,10198,10158,10119,
10079,10039,10000,9960,9920,9880,9840,9799,
9759,9719,9678,9638,9597,9556,9515,9474,
9433,9392,9351,9310,9268,9227,9185,9143,
9102,9060,9018,8976,8934,8892,8849,8807,
8765,8722,8680,8637,8594,8551,8508,8465,
8422,8379,8336,8293,8249,8206,8162,8119,
8075,8031,7987,7943,7899,7855,7811,7767,
7723,7678,7634,7589,7545,7500,7455,7411,
7366,7321,7276,7231,7186,7140,7095,7050,
7004,6959,6913,6868,6822,6776,6731,6685,
6639,6593,6547,6501,6454,6408,6362,6316,
6269,6223,6176,6130,6083,6036,5990,5943,
5896,5849,5802,5755,5708,5661,5613,5566,
5519,5472,5424,5377,5329,5282,5234,5186,
5139,5091,5043,4995,4947,4899,4851,4803,
4755,4707,4659,4611,4563,4514,4466,4418,
4369,4321,4272,4224,4175,4126,4078,4029,
3980,3932,3883,3834,3785,3736,3687,3638,
3589,3540,3491,3442,3393,3344,3294,3245,
3196,3146,3097,3048,2998,2949,2899,2850,
2800,2751,2701,2652,2602,2553,2503,2453,
2403,2354,2304,2254,2204,2155,2105,2055,
2005,1955,1905,1855,1805,1755,1705,1655,
1605,1555,1505,1455,1405,1355,1305,1255,
1205,1155,1104,1054,1004,954,904,854,
803,753,703,653,603,552,502,452,
402,351,301,251,201,150,100,50,
0,-50,-101,-151,-201,-251,-302,-352,
-402,-452,-503,-553,-603,-653,-703,-754,
-804,-854,-904,-954,-1005,-1055,-1105,-1155,
-1205,-1255,-1305,-1356,-1406,-1456,-1506,-1556,
-1606,-1656,-1706,-1756,-1806,-1856,-1906,-1956,
-2006,-2055,-2105,-2155,-2205,-2255,-2304,-2354,
-2404,-2454,-2503,-2553,-2603,-2652,-2702,-2751,
-2801,-2850,-2900,-2949,-2999,-3048,-3098,-3147,
-3196,-3246,-3295,-3344,-3393,-3442,-3491,-3541,
-3590,-3639,-3688,-3737,-3786,-3834,-3883,-3932,
-3981,-4030,-4078,-4127,-4176,-4224,-4273,-4321,
-4370,-4418,-4466,-4515,-4563,-4611,-4660,-4708,
-4756,-4804,-4852,-4900,-4948,-4996,-5044,-5091,
-5139,-5187,-5235,-5282,-5330,-5377,-5425,-5472,
-5519,-5567,-5614,-5661,-5708,-5755,-5802,-5849,
-5896,-5943,-5990,-6037,-6083,-6130,-6177,-6223,
-6270,-6316,-6362,-6409,-6455,-6501,-6547,-6593,
-6639,-6685,-6731,-6777,-6823,-6868,-6914,-6959,
-7005,-7050,-7096,-7141,-7186,-7231,-7276,-7321,
-7366,-7411,-7456,-7501,-7545,-7590,-7634,-7679,
-7723,-7767,-7812,-7856,-7900,-7944,-7988,-8032,
-8075,-8119,-8163,-8206,-8250,-8293,-8336,-8380,
-8423,-8466,-8509,-8552,-8595,-8637,-8680,-8723,
-8765,-8808,-8850,-8892,-8934,-8976,-9018,-9060,
-9102,-9144,-9186,-9227,-9269,-9310,-9351,-9393,
-9434,-9475,-9516,-9557,-9597,-9638,-9679,-9719,
-9760,-9800,-9840,-9880,-9920,-9960,-10000,-10040,
-10080,-10119,-10159,-10198,-10237,-10277,-10316,-10355,
-10394,-10432,-10471,-10510,-10548,-10587,-10625,-10663,
-10701,-10739,-10777,-10815,-10853,-10890,-10928,-10965,
-11002,-11040,-11077,-11114,-11151,-11187,-11224,-11261,
-11297,-11333,-11370,-11406,-11442,-11478,-11514,-11549,
-11585,-11620,-11656,-11691,-11726,-11761,-11796,-11831,
-11866,-11900,-11935,-11969,-12003,-12038,-12072,-12106,
-12139,-12173,-12207,-12240,-12273,-12307,-12340,-12373,
-12406,-12439,-12471,-12504,-12536,-12568,-12601,-12633,
-12665,-12696,-12728,-12760,-12791,-12823,-12854,-12885,
-12916,-12947,-12977,-13008,-13039,-13069,-13099,-13129,
-13159,-13189,-13219,-13249,-13278,-13308,-13337,-13366,
-13395,-13424,-13453,-13481,-13510,-13538,-13566,-13594,
-13622,-13650,-13678,-13706,-13733,-13760,-13788,-13815,
-13842,-13868,-13895,-13922,-13948,-13974,-14001,-14027,
-14053,-14078,-14104,-14130,-14155,-14180,-14205,-14230,
-14255,-14280,-14304,-14329,-14353,-14377,-14401,-14425,
-14449,-14473,-14496,-14519,-14543,-14566,-14589,-14611,
-14634,-14657,-14679,-14701,-14723,-14745,-14767,-14789,
-14811,-14832,-14853,-14874,-14895,-14916,-14937,-14958,
-14978,-14998,-15018,-15038,-15058,-15078,-15098,-15117,
-15136,-15156,-15175,-15193,-15212,-15231,-15249,-15268,
-15286,-15304,-15322,-15339,-15357,-15374,-15392,-15409,
-15426,-15443,-15459,-15476,-15492,-15509,-15525,-15541,
-15557,-15572,-15588,-15603,-15618,-15634,-15649,-15663,
-15678,-15693,-15707,-15721,-15735,-15749,-15763,-15777,
-15790,-15803,-15817,-15830,-15842,-15855,-15868,-15880,
-15893,-15905,-15917,-15928,-15940,-15952,-15963,-15974,
-15985,-15996,-16007,-16018,-16028,-16039,-16049,-16059,
-16069,-16078,-16088,-16097,-16107,-16116,-16125,-16134,
-16142,-16151,-16159,-16167,-16175,-16183,-16191,-16199,
-16206,-16213,-16221,-16228,-16234,-16241,-16248,-16254,
-16260,-16266,-16272,-16278,-16284,-16289,-16294,-16300,
-16305,-16309,-16314,-16319,-16323,-16327,-16331,-16335,
-16339,-16343,-16346,-16350,-16353,-16356,-16359,-16361,
-16364,-16366,-16368,-16370,-16372,-16374,-16376,-16377,
-16379,-16380,-16381,-16382,-16382,-16383,-16383,-16383,
-16384,-16383,-16383,-16383,-16382,-16382,-16381,-16380,
-16379,-16377,-16376,-16374,-16372,-16370,-16368,-16366,
-16364,-16361,-16359,-16356,-16353,-16350,-16346,-16343,
-16339,-16335,-16331,-16327,-16323,-16319,-16314,-16309,
-16305,-16300,-16294,-16289,-16284,-16278,-16272,-16266,
-16260,-16254,-16248,-16241,-16234,-16228,-16221,-16213,
-16206,-16199,-16191,-16183,-16175,-16167,-16159,-16151,
-16142,-16134,-16125,-16116,-16107,-16097,-16088,-16078,
-16069,-16059,-16049,-16039,-16028,-16018,-16007,-15996,
-15985,-15974,-15963,-15952,-15940,-15928,-15917,-15905,
-15893,-15880,-15868,-15855,-15842,-15830,-15817,-15803,
-15790,-15777,-15763,-15749,-15735,-15721,-15707,-15693,
-15678,-15663,-15649,-15634,-15618,-15603,-15588,-15572,
-15557,-15541,-15525,-15509,-15492,-15476,-15459,-15443,
-15426,-15409,-15392,-15374,-15357,-15339,-15322,-15304,
-15286,-15268,-15249,-15231,-15212,-15193,-15175,-15156,
-15136,-15117,-15098,-15078,-15058,-15038,-15018,-14998,
-14978,-14958,-14937,-14916,-14895,-14874,-14853,-14832,
-14811,-14789,-14767,-14745,-14723,-14701,-14679,-14657,
-14634,-14611,-14589,-14566,-14543,-14519,-14496,-14473,
-14449,-14425,-14401,-14377,-14353,-14329,-14304,-14280,
-14255,-14230,-14205,-14180,-14155,-14130,-14104,-14078,
-14053,-14027,-14001,-13974,-13948,-13922,-13895,-13868,
-13842,-13815,-13788,-13760,-13733,-13706,-13678,-13650,
-13622,-13594,-13566,-13538,-13510,-13481,-13453,-13424,
-13395,-13366,-13337,-13308,-13278,-13249,-13219,-13189,
-13159,-13129,-13099,-13069,-13039,-13008,-12977,-12947,
-12916,-12885,-12854,-12823,-12791,-12760,-12728,-12696,
-12665,-12633,-12601,-12568,-12536,-12504,-12471,-12439,
-12406,-12373,-12340,-12307,-12273,-12240,-12207,-12173,
-12139,-12106,-12072,-12038,-12003,-11969,-11935,-11900,
-11866,-11831,-11796,-11761,-11726,-11691,-11656,-11620,
-11585,-11549,-11514,-11478,-11442,-11406,-11370,-11333,
-11297,-11261,-11224,-11187,-11151,-11114,-11077,-11040,
-11002,-10965,-10928,-10890,-10853,-10815,-10777,-10739,
-10701,-10663,-10625,-10587,-10548,-10510,-10471,-10432,
-10394,-10355,-10316,-10277,-10237,-10198,-10159,-10119,
-10080,-10040,-10000,-9960,-9920,-9880,-9840,-9800,
-9760,-9719,-9679,-9638,-9597,-9557,-9516,-9475,
-9434,-9393,-9351,-9310,-9269,-9227,-9186,-9144,
-9102,-9060,-9018,-8976,-8934,-8892,-8850,-8808,
-8765,-8723,-8680,-8637,-8595,-8552,-8509,-8466,
-8423,-8380,-8336,-8293,-8250,-8206,-8163,-8119,
-8075,-8032,-7988,-7944,-7900,-7856,-7812,-7767,
-7723,-7679,-7634,-7590,-7545,-7501,-7456,-7411,
-7366,-7321,-7276,-7231,-7186,-7141,-7096,-7050,
-7005,-6959,-6914,-6868,-6823,-6777,-6731,-6685,
-6639,-6593,-6547,-6501,-6455,-6409,-6362,-6316,
-6270,-6223,-6177,-6130,-6083,-6037,-5990,-5943,
-5896,-5849,-5802,-5755,-5708,-5661,-5614,-5567,
-5519,-5472,-5425,-5377,-5330,-5282,-5235,-5187,
-5139,-5091,-5044,-4996,-4948,-4900,-4852,-4804,
-4756,-4708,-4660,-4611,-4563,-4515,-4466,-4418,
-4370,-4321,-4273,-4224,-4176,-4127,-4078,-4030,
-3981,-3932,-3883,-3834,-3786,-3737,-3688,-3639,
-3590,-3541,-3491,-3442,-3393,-3344,-3295,-3246,
-3196,-3147,-3098,-3048,-2999,-2949,-2900,-2850,
-2801,-2751,-2702,-2652,-2603,-2553,-2503,-2454,
-2404,-2354,-2304,-2255,-2205,-2155,-2105,-2055,
-2006,-1956,-1906,-1856,-1806,-1756,-1706,-1656,
-1606,-1556,-1506,-1456,-1406,-1356,-1305,-1255,
-1205,-1155,-1105,-1055,-1005,-954,-904,-854,
-804,-754,-703,-653,-603,-553,-503,-452,
-402,-352,-302,-251,-201,-151,-101,-50,
};


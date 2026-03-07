#include "unifac_data.h"

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_water_subgroups[] = {
	{16, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_chloroform_subgroups[] = {
	{50, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_methane_subgroups[] = {
	{0, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_methanol_subgroups[] = {
	{0, 1},
	{14, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_ethylene_subgroups[] = {
	{4, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_ethane_subgroups[] = {
	{0, 2},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_ethanol_subgroups[] = {
	{0, 1},
	{1, 1},
	{14, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_propylene_subgroups[] = {
	{0, 1},
	{1, 1},
	{2, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_acetone_subgroups[] = {
	{0, 1},
	{18, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_propane_subgroups[] = {
	{0, 2},
	{1, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_propadiene_subgroups[] = {
	{1, 2},
	{6, 2},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_n_propanol_subgroups[] = {
	{0, 1},
	{1, 2},
	{14, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_i_propanol_subgroups[] = {
	{0, 2},
	{2, 1},
	{14, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_ethylene_glycol_subgroups[] = {
	{1, 2},
	{14, 2},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_n_butane_subgroups[] = {
	{0, 2},
	{1, 2},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_i_butane_subgroups[] = {
	{0, 3},
	{2, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_butene_1_subgroups[] = {
	{0, 1},
	{1, 2},
	{2, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_n_butanol_subgroups[] = {
	{0, 1},
	{1, 3},
	{14, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_i_butanol_subgroups[] = {
	{0, 2},
	{1, 1},
	{2, 1},
	{14, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_n_pentane_subgroups[] = {
	{0, 2},
	{1, 3},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_i_pentane_subgroups[] = {
	{0, 3},
	{1, 1},
	{2, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_benzene_subgroups[] = {
	{9, 6},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_n_hexane_subgroups[] = {
	{0, 2},
	{1, 4},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_toluene_subgroups[] = {
	{9, 5},
	{11, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_n_heptane_subgroups[] = {
	{0, 2},
	{1, 5},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_styrene_subgroups[] = {
	{9, 5},
	{10, 1},
	{4, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_ethylbenzene_subgroups[] = {
	{9, 5},
	{12, 1},
	{0, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_n_octane_subgroups[] = {
	{0, 2},
	{1, 6},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_diethylbenzene_subgroups[] = {
	{9, 4},
	{12, 2},
	{0, 2},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_n_decane_subgroups[] = {
	{0, 2},
	{1, 8},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_diphenyl_subgroups[] = {
	{9, 10},
	{10, 2},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_pentanol_1_subgroups[] = {
	{0, 1},
	{1, 4},
	{14, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_dodecane_subgroups[] = {
	{0, 2},
	{1, 10},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_chloro_benzene_subgroups[] = {
	{9, 5},
	{10, 1},
};

static const FpropsUNIFACComponentSubgroupSource fprops_unifac_comp_2_hexene_cis_subgroups[] = {
	{0, 2},
	{5, 1},
	{1, 2},
};

const FpropsUNIFACComponentSource fprops_unifac_orig_2003_components[] = {
	{"water", "H2O", 1, fprops_unifac_comp_water_subgroups, 0.92, 1.4, 647.3, 22120000, 1, -7.76451, 1.45838, -2.7758, -1.23303, 298.15, 101325, -242000, -228800, 32.24, 0.001924, 1.055e-05, -3.596e-09, 0.344, 0.235, 1.80511022044e-05, 293},
	{"chloroform", "CHCl3", 1, fprops_unifac_comp_chloroform_subgroups, 2.87, 2.41, 536.4, 5370000, 1, -6.95546, 1.16625, -2.1397, -3.44421, 298.15, 101325, -101300, -68580, 24, 0.1893, -0.0001841, 6.657e-08, 0.218, 0.293, 8.01732706514e-05, 293.15},
	{"methane", "CH4", 1, fprops_unifac_comp_methane_subgroups, 0.9011, 0.848, 190.4, 4600000, 1, -6.00435, 1.1885, -0.83408, -1.22833, 298.15, 101325, -74900, -50870, 19.25, 0.05213, 1.197e-05, -1.132e-08, 0.011, 0.288, 3.77482352941e-05, 111.7},
	{"methanol", "CH3OH", 2, fprops_unifac_comp_methanol_subgroups, 1.9011, 2.048, 512.6, 8090000, 1, -8.54796, 0.76982, -3.1085, 1.54481, 298.15, 101325, -201330, -162600, 21.15, 0.07092, 2.587e-05, -2.852e-08, 0.556, 0.224, 4.05082174463e-05, 293},
	{"ethylene", "C2H4", 1, fprops_unifac_comp_ethylene_subgroups, 1.3454, 1.176, 282.4, 5040000, 1, -6.32055, 1.16819, -1.55935, -1.83552, 298.15, 101325, 52340, 68160, 3.806, 0.1566, -8.348e-05, 1.755e-08, 0.089, 0.28, 4.86204506066e-05, 163},
	{"ethane", "C2H6", 1, fprops_unifac_comp_ethane_subgroups, 1.8022, 1.696, 305.4, 4880000, 1, -6.34307, 1.0163, -1.19116, -2.03539, 298.15, 101325, -84740, -32950, 5.409, 0.1781, -6.938e-05, 8.713e-09, 0.099, 0.285, 5.48722627737e-05, 183},
	{"ethanol", "C2H5OH", 3, fprops_unifac_comp_ethanol_subgroups, 2.5755, 2.588, 513.9, 6140000, 1, -8.51838, 0.34163, -5.73683, 8.32581, 298.15, 101325, -235000, -168400, 9.014, 0.2141, -8.39e-05, 1.373e-09, 0.644, 0.24, 5.83891001267e-05, 293.15},
	{"propylene", "C3H6", 3, fprops_unifac_comp_propylene_subgroups, 2.0224, 1.616, 364.9, 4600000, 1, -6.64231, 1.21857, -1.81005, -2.48212, 298.15, 101325, 20430, 62760, 3.71, 0.2345, -0.000116, 2.205e-08, 0.144, 0.274, 6.87598039216e-05, 223},
	{"acetone", "(CH3)2CO", 2, fprops_unifac_comp_acetone_subgroups, 2.5735, 2.336, 508.1, 4700000, 1, -7.45514, 1.202, -2.43926, -3.3559, 298.15, 101325, -217700, -153200, 6.301, 0.2606, -0.0001253, 2.038e-08, 0.304, 0.232, 7.35189873418e-05, 293.15},
	{"propane", "C3H8", 2, fprops_unifac_comp_propane_subgroups, 2.4766, 2.236, 369.8, 4250000, 1, -6.72219, 1.33236, -2.13868, -1.38551, 298.15, 101325, -103900, -23490, -4.224, 0.3063, -0.0001586, 3.215e-08, 0.153, 0.281, 7.57628865979e-05, 231},
	{"propadiene", "C3H4", 2, fprops_unifac_comp_propadiene_subgroups, 3.5834, 3.056, 393, 5470000, 3, 6.5361, 1054.72, -77.08, 0, 298.15, 101325, 192300, 202500, 9.906, 0.1977, -0.0001182, 2.782e-08, 0.313, 0.271, 6.08890577508e-05, 238},
	{"n_propanol", "C3H7OH", 3, fprops_unifac_comp_n_propanol_subgroups, 3.2499, 3.128, 536.7, 5170000, 1, -8.05594, 0.0425183, -7.51296, 6.89004, 298.15, 101325, -256600, -161900, 2.47, 0.3325, -0.0001855, 4.296e-08, 0.623, 0.253, 7.47462686567e-05, 293},
	{"i_propanol", "(CH3)2CHOH", 3, fprops_unifac_comp_i_propanol_subgroups, 3.2491, 3.124, 508.3, 4760000, 1, -8.16927, -0.0943213, -8.1004, 7.85, 298.15, 101325, -272600, -117700, 32.43, 0.1885, 6.406e-05, -9.261e-08, 0.665, 0.248, 7.64580152672e-05, 293},
	{"ethylene_glycol", "C2H6O2", 2, fprops_unifac_comp_ethylene_glycol_subgroups, 3.3488, 3.48, 645, 7700000, 3, 13.6299, 6022.18, -28.25, 0, 298.15, 101325, -389600, -304700, 35.7, 0.2483, -0.0001497, 3.01e-08, 1.05725, 0.27, 5.57172351885e-05, 293},
	{"n_butane", "C4H10", 2, fprops_unifac_comp_n_butane_subgroups, 3.151, 2.776, 425.2, 3800000, 1, -6.88709, 1.15157, -1.99873, -3.13003, 298.15, 101325, -126200, -16100, 9.487, 0.3313, -0.0001108, -2.822e-09, 0.199, 0.274, 0.000100386873921, 293},
	{"i_butane", "(CH3)3CH", 2, fprops_unifac_comp_i_butane_subgroups, 3.1502, 2.772, 408.2, 3650000, 1, -6.95579, 1.5009, -2.52717, -1.49776, 298.15, 101325, -134600, -20900, -1.39, 0.3847, -0.0001846, 2.895e-08, 0.183, 0.283, 0.000104351885099, 293},
	{"butene_1", "C4H8", 3, fprops_unifac_comp_butene_1_subgroups, 2.6968, 2.156, 419.6, 4020000, 1, -6.88204, 1.27051, -2.26284, -2.61632, 298.15, 101325, -37500, 71340, -2.994, 0.3532, -0.000199, 4.463e-08, 0.191, 0.277, 9.42991596639e-05, 293},
	{"n_butanol", "C4H9OH", 3, fprops_unifac_comp_n_butanol_subgroups, 3.9243, 3.668, 563.1, 4420000, 1, -8.00756, 0.53783, -9.3424, 6.68692, 298.15, 101325, -274900, -150900, 3.266, 0.418, -0.0002242, 4.685e-08, 0.593, 0.259, 9.15098765432e-05, 293},
	{"i_butanol", "(CH3)2CHCH2OH", 4, fprops_unifac_comp_i_butanol_subgroups, 3.9235, 3.664, 547.8, 4300000, 3, 10.251, 2874.73, -100.3, 0, 298.15, 101325, -283400, -167400, -7.708, 0.4689, -0.0002884, 7.231e-08, 0.592, 0.257, 9.24226932668e-05, 293},
	{"n_pentane", "C5H12", 2, fprops_unifac_comp_n_pentane_subgroups, 3.8254, 3.316, 469.7, 3370000, 1, -7.28936, 1.53679, -3.08367, -1.02456, 298.15, 101325, -146500, -8370, -3.626, 0.4873, -0.000258, 5.305e-08, 0.251, 0.263, 0.000115257188498, 293},
	{"i_pentane", "C2H5CH(CH3)2", 3, fprops_unifac_comp_i_pentane_subgroups, 3.8246, 3.312, 460.4, 3390000, 1, -7.12727, 1.38996, -2.54302, -2.45657, 298.15, 101325, -154600, -14820, -9.525, 0.5066, -0.0002729, 5.723e-08, 0.227, 0.271, 0.000116372580645, 293},
	{"benzene", "C6H6", 1, fprops_unifac_comp_benzene_subgroups, 3.1878, 2.4, 562.2, 4890000, 1, -6.98273, 1.33213, -2.62863, -3.33399, 298.15, 101325, 82980, 129700, -33.92, 0.4739, -0.0003017, 7.13e-08, 0.212, 0.271, 8.82644067797e-05, 289},
	{"n_hexane", "C6H14", 2, fprops_unifac_comp_n_hexane_subgroups, 4.4998, 3.856, 507.5, 3010000, 1, -7.46765, 1.44211, -3.28222, -2.50941, 298.15, 101325, -167300, -167, -4.413, 0.582, -0.0003119, 6.494e-08, 0.299, 0.264, 0.000130770864947, 293},
	{"toluene", "C6H5CH3", 2, fprops_unifac_comp_toluene_subgroups, 3.9228, 2.968, 591.8, 4100000, 1, -7.28607, 1.38091, -2.83433, -2.79168, 298.15, 101325, 50030, 122100, -24.35, 0.5125, -0.0002765, 4.911e-08, 0.263, 0.263, 0.000106275663206, 293},
	{"n_heptane", "C7H16", 2, fprops_unifac_comp_n_heptane_subgroups, 5.1742, 4.396, 540.3, 2740000, 1, -7.67468, 1.37068, -3.5362, -3.20243, 298.15, 101325, -187900, 8000, -5.146, 0.6762, -0.0003651, 7.658e-08, 0.349, 0.263, 0.000146498538012, 293},
	{"styrene", "C6H5C2H3", 3, fprops_unifac_comp_styrene_subgroups, 4.3671, 3.296, 647, 3990000, 1, -7.15981, 1.78861, -5.10359, 1.63749, 298.15, 101325, 147500, 213900, -28.25, 0.6159, -0.0004023, 9.935e-08, 0.257, 0.26, 0.000114958057395, 293.15},
	{"ethylbenzene", "C6H5C2H5", 3, fprops_unifac_comp_ethylbenzene_subgroups, 4.5972, 3.508, 617.2, 3600000, 1, -7.48645, 1.45488, -3.37538, -2.23048, 298.15, 101325, 29810, 130700, -43.1, 0.7072, -0.0004811, 1.301e-07, 0.302, 0.262, 0.0001224544406, 293},
	{"n_octane", "C8H18", 2, fprops_unifac_comp_n_octane_subgroups, 5.8486, 4.936, 568.8, 2490000, 1, -7.91211, 1.38007, -3.80435, -4.50132, 298.15, 101325, -208600, 16400, -6.096, 0.7712, -0.0004195, 8.855e-08, 0.398, 0.259, 0.000162492176387, 293},
	{"diethylbenzene", "C6H4(C2H5)2", 3, fprops_unifac_comp_diethylbenzene_subgroups, 6.0066, 4.616, 657.9, 2800000, 1, -8.11413, 1.77697, -4.4396, -1.47477, 298.15, 101325, -22270, 138000, -37.42, 0.8671, -0.000556, 1.411e-07, 0.404, 0.25, 0.000155709976798, 293},
	{"n_decane", "C10H22", 2, fprops_unifac_comp_n_decane_subgroups, 7.1974, 6.016, 617.7, 2120000, 1, -8.56523, 1.97756, -5.81971, -0.29982, 298.15, 101325, -249800, 33240, -7.913, 0.9609, -0.0005288, 1.131e-07, 0.489, 0.249, 0.000194912328767, 293},
	{"diphenyl", "(C6H5)2", 2, fprops_unifac_comp_diphenyl_subgroups, 6.0434, 4.24, 789, 3850000, 1, -7.674, 1.23008, -3.67908, -2.29172, 298.15, 101325, 182200, 280300, -97.07, 1.106, -0.0008855, 2.79e-07, 0.372, 0.295, 0.00015576969697, 347},
	{"pentanol_1", "C5H12O", 3, fprops_unifac_comp_pentanol_1_subgroups, 4.5987, 4.208, 588.2, 3910000, 1, -8.97725, 2.99791, -12.9596, 8.84205, 298.15, 101325, -298900, -146100, 3.869, 0.5045, -0.0002639, 5.12e-08, 0.579, 0.26, 0.000108159509202, 293},
	{"dodecane", "C12H26", 2, fprops_unifac_comp_dodecane_subgroups, 8.5462, 7.096, 658.2, 1820000, 2, 77.628, 10012.5, -9.236, 10030, 298.15, 101325, -291100, 50070, -9.328, 1.149, -0.0006347, 1.359e-07, 0.575, 0.24, 0.000227727272727, 293},
	{"chloro_benzene", "C6H5Cl", 2, fprops_unifac_comp_chloro_benzene_subgroups, 3.0217, 2.12, 632.4, 4520000, 1, -7.587, 2.26551, -4.09418, 0.17038, 298.15, 101325, 51870, 99230, -33.89, 0.5631, -0.0004522, 1.426e-07, 0.249, 0.265, 0.00010177124774, 293},
	{"2_hexene_cis", "C6H12", 3, fprops_unifac_comp_2_hexene_cis_subgroups, 4.2677, 3.643, 518, 3280000, 3, 9.5855, 2897.97, -39.3, 0, 298.15, 101325, -52380, 76280, -9.81, 0.5309, -0.000272, 4.83e-08, 0.256, 0.27, 0.000122506550218, 293},
};

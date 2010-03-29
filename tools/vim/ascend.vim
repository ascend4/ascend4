" Vim syntax file
" Language: ASCEND Modelling Language (.a4l/.a4c)
" Author: Edgar Simo <bobbens@gmail.com>
" Last Change: 29 March 2010
" Remark: Syntax highlighting for the ASCEND Language.
"
" How to use this:
" * Put ascend.vim (this file) under ~/.vim/syntax (or similar directory for
"   your system - usually C:\Program Files\Vim\vimfiles\syntax on Windows).
" * In your .vimrc, add these lines:
"     au BufRead,BufNewFile *.a4c set syntax=ascend
"     au BufRead,BufNewFile *.a4l set syntax=ascend
" Thereafter, any .des files you edit in (g)vim will use syntax highlighting.

" Quit when syntax hilighting already loaded
if exists("b:current_syntax")
   finish
endif

syn case match

" Regex matching
syn region a4lComment start="(\*" end="\*)"
syn match a4lString "\"[^\"]*\""
syn match a4lChar "'.'"
syn match a4lUnits "{[^}]*}"
" Number and float definitions stolen from c.vim
syn match a4lNumber "\d\+\(u\=l\{0,2}\|ll\=u\)\>"
syn match a4lFloat "\d\+\.\d*\(e[-+]\=\d\+\)\=[fl]\="
syn match a4lFloat "\.\d\+\(e[-+]\=\d\+\)\=[fl]\=\>"
syn match a4lFloat "\d\+e[-+]\=\d\+[fl]\=\>"

" Static keywords
syn keyword a4lKeyword ADD AND ALIASES ARE_ALIKE ARE_THE_SAME ARE_NOT_THE_SAME ASSERT ATOM CALL CASE CARD CHECK CHOICE CONSTANT CONDITIONAL CREATE DATA DEFAULT DEFINITION DIMENSION DIMENSIONLESS DO ELSE END EXTERNAL FIX FOR FREE IF IMPORT IN INPUT INTERSECTION IS_A IS_REFINED_TO MAX MAXIMIZE METHOD METHODS MIN MINIMIZE MODEL NOTES OF OR OTHERWISE OUTPUT PROVIDE PROD REPLACE REQUIRE REFINES RUN SATISFIED SELECT SELF STUDY SUCH_THAT SUM SWITCH THEN UNIVERSAL UNION UNITS USE WHEN WHERE WITH_VALUE WILL_BE WILL_BE_THE_SAME WILL_NOT_BE_THE_SAME  
syn keyword a4lTypes catch_Word_model relation logic_relation solver_var boolean_var generic_real solver_int solver_binary solver_semi  constant critical_compressibility acentric_factor UNIFAC_size Wilson_constant vapor_pressure_constant factor_constant molar_weight_constant atomic_mass_constant temperature_constant boiling_temperature critical_temperature reference_temperature UNIFAC_a pressure_constant molar_volume_constant critical_volume reference_molar_volume reference_mass_density molar_energy_constant reference_molar_energymolar_gas_constant gravity_constant circle_constant speed_of_light planck_constantboolean_start_true boolean_start_false start_true start_false bound_width scaling_constant ode_counter obs_counter real_parameter length_parameter positive_variable factor variable fraction positive_factor small_factor small_positive_factor reduced_pressure exp_sub power_sub temperature inverse_temperature delta_temperature force force_per_length force_per_volume surface_tension pressure pressure_rate delta_pressure k_constant vapor_pressure youngs_modulus pressure_per_length molar_mass mass mole_scale mole mass_rate mass_rate_constant mass_flux mass_rate_rate mass_rate_per_length molar_rate_scale molar_rate conc_rate mole_fraction mass_fraction molar_volume volume_scale volume volume_rate_scale volume_rate volume_rate_square volume_expansivity molar_density mass_density molar_energy energy_scale energy energy_per_volume energy_rate_scale energy_rate power_per_length power_per_volume power_per_area power_per_temperature irradiance irradiation molar_heat_capacity molar_energy_rate molar_entropy entropy entropy_rate partition_coefficient relative_volatility monetary_unit cost_per_volume cost_per_mass cost_per_mass_constant cost_per_mole cost_per_time cost_per_energy cost_per_mass_per_distance_constant distance distance_constant area inverse_area angle solid_angle time speed acceleration frequency stiffness viscosity kinematic_viscosity thermal_conductivity diffusivity voltage resistance current capacitance inductance magnetic_field electric_field delta_distance delta_area temperature_rate delta_mass delta_mole delta_mass_rate delta_molar_rate delta_volume_rate density_rate delta_energy_rate delta_molar_energy_rate delta_entropy delta_entropy_rate mass_sec mole_sec rate deflection second_moment_of_inertia polar_moment_of_inertia second_moment_of_area_constant length_constant area_constant moment stress specific_gas_constant mass_density_constant heat_transfer_coefficient specific_enthalpy specific_entropy specific_heat_capacity heat_capacity specific_volume specific_energy delta_specific_energy specific_power delta_specific_power specific_energy_rate specific_enthalpy_rate ua_value thermal_resistance R_value pressure_per_temperature energy_rate_per_length energy_flux capacity_rate thermo_state real_constant integer_constant symbol_constant boolean_constant real integer symbol boolean set pltmodel plt_point plt_curve plt_plot_symbol plt_plot_integer 
syn keyword a4lMethods on_load default_self specify reset values ClearAll bound_self default_all bound_all self_test scale_self check_self check_all scale_all default 
syn keyword a4lBool TRUE FALSE

" Colours
hi link a4lComment   Comment
hi link a4lString    String
hi link a4lChar      Character
hi link a4lUnits     Label
hi link a4lNumber    Number
hi link a4lFloat     Float
hi link a4lKeyword   Statement
hi link a4lTypes     Type
hi link a4lMethods   Identifier
hi link a4lBool      Constant

" Mark as loaded
let b:current_syntax = "ascend"

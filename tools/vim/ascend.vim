" Vim syntax file
" Language: ASCEND Language files (.a4l/.a4c)
" Maintainer: Edgar Simo <bobbens@gmail.com>
" Last Change: 30 Mar 2010 by Wojciech Mandziuk <wojciech.mandziuk@gmail.com>
" Remark: Syntax highlighting for the ASCEND Language.
"
" How to use this:
" * Put ascend.vim (this file) under ~/.vim/syntax (or similar directory for
"   your system - usually C:\Program Files\Vim\vimfiles\syntax on Windows).
" * In your .vimrc, add these lines:
"     au BufRead,BufNewFile *.a4c set syntax=ascend
"     au BufRead,BufNewFile *.a4l set syntax=ascend
" Thereafter, any .a4c or a4l you edit in (g)vim will use syntax highlighting.

" Quit when syntax hilighting already loaded
if exists("b:current_syntax")
   finish
endif


" Regex matching
syn case match
syn region a4lComment start="(\*" end="\*)"
syn region a4lString start="\"" end="\""
syn region a4lUnits start="{" end="}"
syn match a4lChar "'.'"

" Number and float definitions taken from lua.vim
syn case ignore
syn match a4lNumber "\<\d\+\>"
" floating point number, with dot, optional exponent
syn match a4lFloat  "\<\d\+\.\d*\%(e[-+]\=\d\+\)\=\>"
" floating point number, starting with a dot, optional exponent
syn match a4lFloat  "\.\d\+\%(e[-+]\=\d\+\)\=\>"
" floating point number, without dot, with exponent
syn match a4lFloat  "\<\d\+e[-+]\=\d\+\>"



" Static keywords
syn case match
syn keyword a4lKeyword ADD AND ALIASES ARE_ALIKE ARE_THE_SAME ARE_NOT_THE_SAME ASSERT ATOM CALL CASE CARD CHECK CHOICE CONSTANT CONDITIONAL CREATE DATA DEFAULT DEFINITION DIMENSION DIMENSIONLESS DO ELSE END EXTERNAL FIX FOR FREE IF IMPORT IN INPUT INTERSECTION IS_A IS_REFINED_TO MAX MAXIMIZE METHOD METHODS MIN MINIMIZE MODEL NOTES OF OR OTHERWISE OUTPUT PROVIDE PROD REPLACE REQUIRE REFINES RUN SATISFIED SELECT SELF STUDY SUCH_THAT SUM SWITCH THEN UNIVERSAL UNION UNITS USE WHEN WHERE WITH_VALUE WILL_BE WILL_BE_THE_SAME WILL_NOT_BE_THE_SAME  
syn keyword a4lTypes acceleration acentric_factor angle angular_speed area area_constant atomic_mass_constant avogadro_constant boiling_temperature boolean boolean_constant boolean_start_false boolean_start_true boolean_var bound_width capacitance capacity_rate catch_Word_model circle_constant conc_rate constant cost_per_energy cost_per_mass cost_per_mass_constant cost_per_mass_per_distance_constant cost_per_mole cost_per_time cost_per_volume critical_compressibility critical_pressure critical_temperature critical_volume current deflection delta_area delta_distance delta_energy_rate delta_entropy delta_entropy_rate delta_mass delta_mass_rate delta_molar_energy_rate delta_molar_rate delta_mole delta_pressure delta_specific_energy delta_specific_power delta_temperature delta_volume_rate density_rate diffusivity distance distance_constant electric_field electron_charge electron_mass energy energy_flux energy_per_volume energy_rate energy_rate_per_length energy_rate_scale energy_scale enthalpy_of_formation_constant entropy entropy_rate exp_sub factor factor_constant force force_per_length force_per_volume fraction free_energy_of_formation_constant frequency generic_real gravity_constant heat_capacity heat_of_vaporization_constant heat_transfer_coefficient inductance integer integer_constant inverse_area inverse_temperature irradiance irradiation k_constant kinematic_viscosity length_constant length_parameter logic_relation magnetic_field mass mass_density mass_density_constant mass_flux mass_fraction mass_rate mass_rate_constant mass_rate_per_length mass_rate_rate mass_sec molar_density molar_energy molar_energy_constant molar_energy_rate molar_entropy molar_entropy_constant molar_gas_constant molar_heat_capacity molar_mass molar_rate molar_rate_scale molar_volume molar_volume_constant molar_weight_constant molar_weigth_constant mole mole_fraction mole_scale mole_sec moment momentary_unit monetary_unit obs_counter ode_counter partition_coefficient permeability_constant permittivity_constant planck_constant plt_curve plt_plot_integer plt_plot_symbol plt_point pltmodel polar_moment_of_inertia positive_factor positive_variable power_per_area power_per_length power_per_temperature power_per_volume power_sub pressure pressure_constant pressure_per_length pressure_per_temperature pressure_rate proton_mass R_value rate real real_constant real_parameter reduced_pressure reference_mass_density reference_molar_energy reference_molar_volume reference_pressure reference_temperature relation relative_volatility resistance scaling_constant second_moment_of_area second_moment_of_area_constant second_moment_of_inertia set small_factor small_positive_factor solid_angle solver_binary solver_int solver_semi solver_var specific_energy specific_energy_rate specific_enthalpy specific_enthalpy_rate specific_entropy specific_gas_constant specific_heat_capacity specific_power specific_volume speed speed_of_light start_false start_true stiffness stress surface_tension symbol symbol_constant temperature temperature_constant temperature_rate thermal_conductivity thermal_resistance thermo_state time ua_value UNIFAC_a UNIFAC_size vapor_pressure vapor_pressure_constant variable viscosity voltage volume volume_expansivity volume_rate volume_rate_scale volume_rate_square volume_scale Wilson_constant Wilson_energy_constant youngs_modulus 
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


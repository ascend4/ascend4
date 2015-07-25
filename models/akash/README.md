#GSOC'15 ASCEND : Chemical Engineering specific models
---
## Steam Reformation
####Abstract
Steam reforming is the process of production of Hydrogen(mainly) , Carbon Monoxide and other important products by the reaction of hydrocarbons with steam. In case of methane-steam-reforming, methane is used as the hydrocarbon and as a resultant, syngas (CO + H2) is produced. Our purpose is to write a functional ASCEND model for methane steam reformation.

####Theory
A methane steam reforming system consisting of two reactors:-
1. Steam reforming reaction
2. Water-gas shift reaction

#####1. Steam reforming reaction
Reaction of Natural Gas and steam at high temperature.

######Natural Gas
Although Natural Gas contains many chemical compounds, we will assume that it consists of only CH4. Steam reforming reactions of CH4 occur over a Ni-based catalyst. The reactions are as follows:

Main Reaction: 		CH4 (g) +  H20 (g)   ⇔   CO (g) + 3H2 (g)   ΔHr= 206 KJ/mol

######Conversion
Typically, about 90-95% conversion of methane is achieved by steam reforming in single-pass.
For ~100% conversion, a second steam reformer needs to be used. But we are not considering that in the ASCEND process modeling for the time being.
>Note: In case of Ammonia production, second reformer is fed with direct air (~ 79%N2 + 21% O2), so the nitrogen reacts with the hydrogen produced to give ammonia :- N2 + 3H2 ⇔ 2NH3

######Sweetening of gas
Natural Gas also contains around 1% of H2S. For the Ni catalyst, H2S is a poison and needs to be removed down to few hundred parts per million. This removal is called sweetening of natural gas. Ethanol Amines are used in the absorption of H2S and removal of it.
We are assuming the gas to be already free of H2S.

######Reaction Conditions
Being an endothermic reaction, it is supported at high temperature (800 - 900 ℃). Also a moderate pressure (3 - 25 bar) is preferable.

#####2. Water-gas shift reaction
Significant temperature dependence and the equilibrium constant decreases with an increase in temperature, that is, higher carbon monoxide conversion is observed at lower temperatures.

CO + H2O ⇔  CO2 + H2 

For calculation, same reaction conditions have been assumed.


#####Need for steam reformation?
- **Hydrogen** - Large quantities of hydrogen gas are required in the petrochemical industry. Also, hydrogen gas is used in Hydrogen fuel cells.
- **Carbon Monoxide** - CO is used widely in the production of Methanol. Aldehydes are produced by hydroformylation reaction of alkenes, CO and hydrogen.

######Fugacity
Fugacity is the pressure value needed at a given temperature to make the properties of a non-ideal gas satisfy the equation for an ideal gas.

Fi = fi * Pi

where,  
Fi = Fugacity of ith gas component,  
fi	= fugacity coefficient of ith gas component,  
Pi	= Partial pressure of ith gas component

![Steam Reforming](http://www.digipac.ca/chemical/mtom/contents/chapter4/images/hreforming.gif)

####ASCEND Modeling
1. CH4 (g) +  H20 (g)   ⇔   CO (g) + 3H2 (g)		:	Reaction Coefficient = k_reforming
2. CO + H2O ⇔  CO2 + H2			:	Reaction Coefficient = k_shift

Write basic balances for the two reactions in terms of conversions and initial molar conc./molar flow rate.

Writing reaction coefficients in terms of fugacity:-  
k_reforming	= (fCO * f3H2) / ( fCH4 * fH2O)  
k_shift		= (fCO2 * fH2) / ( fCO * fH2O)  
These calculations are solved in model reformation which calls the reactor model. This is done in file named steam_reformation.a4c

Using SRK equation of states:-  
fugacity coeff.(i)= exp{(Z-1)*Bi /B –ln(Z-B) –A/B* (2Ai0.5/A0.5 – Bi /B)* ln((Z+B)/Z)}
where,  
Ai= 0.42747 aiPr(i)/T2r(i),  
Bi= 0.08664 Pr(i)/Tr(i),  
ai=[1+mi(1-Tr(i)0.5)]2 ,  
mi=0.48 + 1.574 wi - 0.176 wi2,  
A= sum[sum(yi yj Aij)],  
Aij= (Ai Aj)0.5,  
B= sum(yi Bi), Z3 – Z2 + Z*(A-B-B2) - A*B=0  
Pr and Tr are reduced pressure and temperatures

This part is solved by modification of models/kchittur/eos-srk.a4c and new file is saved as akash-eos.a4c.

####References
Acentric Factor : http://webserver.dmt.upm.es/~isidoro/dat1/eGAS.pdf  
Mathematical Model : http://www3.kfupm.edu.sa/catsymp/Symp12th/Data%5CAramco-15.pdf  
Detailed Documentation : https://docs.google.com/document/d/1XMvgZq-kiQaB27iSpOvbnnO875I_VII0BvBUnPTLNls/edit?usp=sharing
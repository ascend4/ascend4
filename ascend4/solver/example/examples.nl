$example 0   ; unsolved,solved,solved
nvars 2
nrels 2
r0 : v0^6 + v1^4 = 17
r1 : 2*v0 + v1 = 4

v0 LB 0
v1 LB 0
v0 UB 10
v1 UB 10

v0 = 4   ; 0   2.47   4
v1 = 2   ; 0   3.87   2

output more important yes
output less important yes
end


$example 1   ;solved
nvars 2
nrels 2

r0 : (v0-4)^2 + (v1-5)^2 = 4
r1 : 2*v0^2 - 16*v0 + 33 - v1 = 0

v0 LB 0
v1 LB 0
v0 UB 10
v1 UB 10

v0 = 0
v1 = 0

output more important yes
output less important yes
end


$example 2   ; solved,solved,solved
nvars 2
nrels 2

r0 : 10000*v0*v1 = 1
r1 : exp(-v0) + exp(-v1) = 1.001

v0 LB 5.49e-6
v0 UB 2.196e-5
v1 LB 4.553
v1 UB 18.21

v0 = 0   ; 0   0   1.8016
v1 = 1   ; 1   0   0

output more important yes
output less important yes
end


$example 3   ; solved
nvars 2
nrels 2

r0 : v1 = 0
r1 : 10*v0/(v0+0.1) + 2*v1^2 = 0

v0 LB -2
v0 UB 2
v1 LB -2
v1 UB 2

v0 = 3
v1 = 1

output more important yes
output less important yes
end


$example 4   ; solved,solved
nvars 3
nrels 3

r0 : 10000*v0*v1 = 1
r1 : exp(-v0) + exp(-v1) = 1.001
r2 : v0*v2^2 = 1

v0 LB 4.553     ;5.49e-6    4.553
v0 UB 18.21     ;2.196e-5   18.21
v1 LB 5.49e-6   ;4.553      5.49e-6
v1 UB 2.196e-5  ;18.21      2.196e-5
v2 LB 0.1655    ;-603       0.1655
v2 UB 0.622     ;-150       0.622

v0 = 0   ; 0   0
v1 = 0   ; 1   0
v2 = 0   ; 1   0

output more important yes
output less important yes
end


$example 5   ; solved
nvars 2
nrels 2

r0 : 10*(v1-v0^2) = 0
r1 : v1 = 1

v0 LB 0.5
v0 UB 2
v1 LB 0.5
v1 UB 2

v0 = 0
v1 = 0

output more important yes
output less important yes
end


$example 6   ; solved
nvars 2
nrels 2

r0 : v0^2 - v1 = 1
r1 : (v0-2)^2 + (v1-0.5)^2 = 1

v0 LB 0
v0 UB 10
v1 LB 0
v1 UB 10

v0 = 0.1
v1 = 2

output more important yes
output less important yes
end


$example 7   ; solved
nvars 4
nrels 4

; v2 = PI, v3 = e
r0 : 0.5*sin(v0*v1) - 0.25*v1/v2 - 0.5*v0 = 0
r1 : (1-0.25/v2)*(exp(2*v0)-v3) + (v3/v2)*v1 - 2*v3*v0 = 0
r2 : v2 = 4*arctan(1)
r3 : v3 = exp(1)

v0 LB 0.25
v0 UB 1
v1 LB 1.5
v1 UB 6.28

v0 = 0.6
v1 = 3

output more important yes
output less important yes
end


$example 8   ;solved
nvars 3
nrels 3

r0 : v0^2 + 2*v1^2 = 4
r1 : v0^2 + v1^2 + v2 = 8
r2 : (v0-1)^2 + (2*v1-2)^2 + (v2-5)^2 = 4

v0 LB 1
v0 UB 4
v1 LB -5
v1 UB 10
v2 LB 2
v2 UB 8

v0 = 1
v1 = 0.7
v2 = 5

output more important yes
output less important yes
end


$example 9   ; solved
nvars 2
nrels 2

r0 : 10*(v1-v0^2) = 0
r1 : 1-v0 = 0

v0 = -1.2
v1 = 1

output more important yes
output less important yes
end


$example 10   ; solved
nvars 4
nrels 4

r0 : v0 + 10*v1 = 0
r1 : sqrt(5)*(v2-v3) = 0
r2 : (v1 - 2*v2)^2 = 0
r3 : sqrt(10)*(v0-v3)^2 = 0

v0 LB -1
v0 UB 1
v1 LB -1
v1 UB 1
v2 LB -1
v2 UB 1
v3 LB -1
v3 UB 1

v0 = 3
v1 = -1
v2 = 0
v3 = 1

output more important yes
output less important yes
end


$example 11   ; solved
nvars 2
nrels 2

r0 : v1 = 1
r1 : 10000*(v1 - 0.2*ln(v0)) = 0

v0 LB 74.206
v0 UB 296.83
v1 LB 0.5
v1 UB 2

v0 = 146
v1 = 0

output more important yes
output less important yes
end


$example 12   ; solved,solved
nvars 2
nrels 2

r0 : v0*v1 = 1
r1 : v0^2 + v1^2 = 3

v0 LB 0
v0 UB 4
v1 LB 0
v1 UB 4

v0 = 2 ; 2   4
v1 = 2 ; 2   2

output more important yes
output less important yes
end


$example 13   ; solved,solved
nvars 2
nrels 2

r0 : v0^2 + v1^2 = 1
r1 : v0 = v1^2

v0 LB 0
v0 UB 4
v1 LB 0
v1 UB 4

v0 = 2 ; 2   4
v1 = 2 ; 2   2

output more important yes
output less important yes
end


$example 14
nvars 2
nrels 2

r0 : v0^2 - v1 = 1
r1 : (v0-v1)^2 + (v1-0.5)^2 = 1

v0 LB 0
v0 UB 10
v1 LB 0
v1 UB 10

v0 = 0.1
v1 = 2

output more important yes
output less important yes
end

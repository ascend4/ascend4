$! This moves and renames various MINOS files into a subdirectory.
$ rename vminos.com     [.opt]minos.*
$ rename vminos.lnk           minos.*
$ rename vminos.mak           minos.*
$ rename vminosl.com    [.opt]minosl.*
$ rename vminosl.lnk          minosl.*
$ rename vminosl.mak          minosl.*
$ rename vminost.com    [.opt]minost.*
$ rename vminost.lnk          minost.*
$ rename vminost.mak          minost.*
$ rename *.spc          [.opt]

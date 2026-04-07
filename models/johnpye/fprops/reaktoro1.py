from reaktoro import SupcrtDatabase

db = SupcrtDatabase("supcrt98.xml")
for sp in ["Ni", "NiO", "H2", "H2O"]:
    props = db.species(sp).props(900, "C", 1, "bar")
    print(sp, props)

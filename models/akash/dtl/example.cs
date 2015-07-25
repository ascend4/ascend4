using Microsoft.VisualBasic;
using System;
using System.Collections;
using System.Collections.Generic;
using DTL.DTL.SimulationObjects.PropertyPackages;

static class Module1
{


	public static void Main()
	{
		Console.WriteLine("DTL Property and Equilibrium calculation example with Water and Ethanol");
		Console.WriteLine(Constants.vbCrLf);

		DTL.Thermodynamics.Calculator dtlc = new DTL.Thermodynamics.Calculator();

		dtlc.Initialize();

		string[] proppacks = dtlc.GetPropPackList();

		string nrtl = proppacks[8];

		PropertyPackage prpp = dtlc.GetPropPackInstance(nrtl);

		string[] compprops = null;

		double T = 0;
		double P = 0;
		string pval = null;

		T = 355;
		//K
		P = 101325;
		//Pa

		compprops = dtlc.GetCompoundConstPropList();

		Console.WriteLine("Ethanol constant properties:" + Constants.vbCrLf);
		foreach (string prop in compprops) {
			pval = dtlc.GetCompoundConstProp("Ethanol", prop);
			Console.WriteLine(prop.PadRight(40) + Constants.vbTab + pval);
		}

		Console.Write(Constants.vbCrLf);
		Console.WriteLine("Press any key to continue...");
		Console.ReadKey(true);

		compprops = dtlc.GetCompoundPDepPropList();

		Console.Write(Constants.vbCrLf);
		Console.WriteLine("Ethanol pressure-dependent properties at P = " + P.ToString("#") + " Pa:" + Constants.vbCrLf);
		foreach (string prop in compprops) {
			pval = dtlc.GetCompoundPDepProp("Ethanol", prop, P);
			Console.WriteLine(prop.PadRight(40) + Constants.vbTab + pval);
		}

		Console.Write(Constants.vbCrLf);
		Console.WriteLine("Press any key to continue...");
		Console.ReadKey(true);

		compprops = dtlc.GetCompoundTDepPropList();

		Console.Write(Constants.vbCrLf);
		Console.WriteLine("Ethanol temperature-dependent properties at T = " + T.ToString("###.##") + " K:" + Constants.vbCrLf);
		foreach (string prop in compprops) {
			pval = dtlc.GetCompoundTDepProp("Ethanol", prop, T);
			Console.WriteLine(prop.PadRight(40) + Constants.vbTab + pval);
		}

		Console.Write(Constants.vbCrLf);
		Console.WriteLine("Press any key to continue...");
		Console.ReadKey(true);

		Console.Write(Constants.vbCrLf);
		Console.WriteLine("Water/Ethanol Interaction Parameters for NRTL model:");
		Console.Write(Constants.vbCrLf);

		DTL.DTL.ClassesBasicasTermodinamica.InteractionParameter ip = dtlc.GetInteractionParameterSet("NRTL", "Water", "Ethanol");
		Console.WriteLine("A12 = " + ip.Parameters("A12").ToString + " cal/mol");
		Console.WriteLine("A21 = " + ip.Parameters("A21").ToString + " cal/mol");
		Console.WriteLine("alpha = " + ip.Parameters("alpha").ToString);

		Console.Write(Constants.vbCrLf);
		Console.WriteLine("Press any key to continue...");
		Console.ReadKey(true);

		Console.Write(Constants.vbCrLf);
		Console.WriteLine("PT Flash of an equimolar mixture of Water and Ethanol at T = " + T.ToString("###.##") + " K and P = " + P.ToString("#") + " Pa:" + Constants.vbCrLf);
		Console.WriteLine("Using NRTL model for equilibrim calculations.");
		Console.Write(Constants.vbCrLf);
		object[,] result2 = dtlc.PTFlash(prpp, 5, P, T, new string[] {
			"Water",
			"Ethanol"
		}, new double[] {
			0.5,
			0.5
		});

		Console.Write(Constants.vbCrLf);
		Console.WriteLine("Flash calculation results:");
		Console.Write(Constants.vbCrLf);

		int i = 0;
		int j = 0;
		string line = null;
		for (i = 0; i <= result2.GetLength(0) - 1; i++) {
			switch (i) {
				case 0:
					line = "Phase Name".PadRight(30) + Constants.vbTab;
					break;
				case 1:
					line = "Phase Mole Fraction in Mixture".PadRight(30) + Constants.vbTab;
					break;
				case 2:
					line = "Water Mole Fraction in Phase".PadRight(30) + Constants.vbTab;
					break;
				case 3:
					line = "Ethanol Mole Fraction in Phase".PadRight(30) + Constants.vbTab;
					break;
				default:
					line = "";
					break;
			}
			for (j = 0; j <= result2.GetLength(1) - 1; j++) {
				if (double.TryParse(result2[i, j].ToString(), out new double())) {
					line += double.Parse(result2[i, j].ToString()).ToString("0.0000").PadRight(10);
				} else {
					line += result2[i, j].ToString().PadRight(10);
				}
			}
			Console.WriteLine(line);
		}

		Console.Write(Constants.vbCrLf);
		Console.WriteLine("Press any key to continue...");
		Console.ReadKey(true);

		Console.Write(Constants.vbCrLf);
		Console.WriteLine("Vapor Phase Mixture Properties at T = " + T.ToString("###.##") + " K and P = " + P.ToString("#") + " Pa:" + Constants.vbCrLf);

		string[] compphaseprops = dtlc.GetPropList();
		object[] values = null;
		foreach (string prop in compphaseprops) {
			values = dtlc.CalcProp(prpp, prop, "Mole", "Vapor", new string[] {
				"Water",
				"Ethanol"
			}, T, P, new double[] {
				double.Parse(result2[2, 0].ToString()),
				double.Parse(result2[3, 0].ToString())
			});
			line = "";
			for (i = 0; i <= values.Length - 1; i++) {
				line += double.Parse(values[i].ToString()).ToString("N6") + Constants.vbTab;
			}
			Console.WriteLine(prop.PadRight(30) + Constants.vbTab + line);
		}

		Console.Write(Constants.vbCrLf);
		Console.WriteLine("Press any key to continue...");
		Console.ReadKey(true);

		Console.Write(Constants.vbCrLf);
		Console.WriteLine("Liquid Phase Mixture Properties at T = " + T.ToString("###.##") + " K and P = " + P.ToString("#") + " Pa:" + Constants.vbCrLf);

		foreach (string prop in compphaseprops) {
			values = dtlc.CalcProp(prpp, prop, "Mole", "Liquid", new string[] {
				"Water",
				"Ethanol"
			}, T, P, new double[] {
				double.Parse(result2[2, 1].ToString()),
				double.Parse(result2[3, 1].ToString())
			});
			line = "";
			for (i = 0; i <= values.Length - 1; i++) {
				line += double.Parse(values[i].ToString()).ToString("N6") + Constants.vbTab;
			}
			Console.WriteLine(prop.PadRight(30) + Constants.vbTab + line);
		}

		Console.Write(Constants.vbCrLf);
		Console.WriteLine("Press any key to exit...");
		Console.ReadKey(true);

	}

}
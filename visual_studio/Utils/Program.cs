using Raylib_cs;

using System.Numerics;
using SixLabors.ImageSharp;
using SixLabors.ImageSharp.PixelFormats;
using System;
using System.Diagnostics;

namespace Dashboard {
	static unsafe class Program {
		static FileStream FS;
		static StreamWriter SW;

		static void WriteLine(string Fmt, params object[] Args) {
			SW.WriteLine(string.Format(Fmt, Args));
		}
		static void WriteLine(string Txt) {
			SW.WriteLine(Txt);
		}

		static void Write(string Fmt, params object[] Args) {
			SW.Write(string.Format(Fmt, Args));
		}


		static void Main(string[] args) {
			ObjLoader.Load("data/models/fish.obj", out Vector3[] VertsArr, out Vector2[] UVsArr);
			Console.WriteLine("Verts: {0}, UVs: {1}", VertsArr.Length, UVsArr.Length);

			FS = File.OpenWrite("out.h");
			SW = new StreamWriter(FS);

			Vector3 Center = Vector3.Zero;

			Vector3 Min = VertsArr[0];
			Vector3 Max = VertsArr[0];
			Vector3 Scale = Vector3.Zero;

			for (int i = 0; i < VertsArr.Length; i++) {
				//Center += VertsArr[i];

				//VertsArr[i] = VertsArr[i] * 30;

				Min = Vector3.Min(Min, VertsArr[i]);
				Max = Vector3.Max(Max, VertsArr[i]);
			}

			Scale = Max - Min;
			Center = Scale / 2;


			Console.WriteLine("Min: {0}\nMax: {1}\nScale: {2}\nCenter: {3}", Min, Max, Scale, Center);

			/*for (int i = 0; i < VertsArr.Length; i++) {
				VertsArr[i] -= Center / 2;
			}*/

			WriteLine("static const float model_verts_obj[] PROGMEM = {");
			Write("    ");

			for (int i = 0; i < VertsArr.Length; i++) {
				Write("{0}f, {1}f, {2}f", VertsArr[i].X, VertsArr[i].Y, VertsArr[i].Z);

				if (i < VertsArr.Length - 1)
					Write(", ");
				else
					Write(" ");
			}

			WriteLine("");
			WriteLine("};");
			WriteLine("");

			WriteLine("static const float model_uvs_obj[] PROGMEM = {");
			Write("    ");

			for (int i = 0; i < UVsArr.Length; i++) {
				Write("{0}f, {1}f", UVsArr[i].X, UVsArr[i].Y);

				if (i < UVsArr.Length - 1)
					Write(", ");
				else
					Write(" ");
			}

			WriteLine("");
			WriteLine("};");
			WriteLine("");

			Image<Rgba32> Img = SixLabors.ImageSharp.Image.Load<Rgba32>("data/fonts/mono10.png");

			WriteLine("static const int font_tex_width = {0};", Img.Width);
			WriteLine("static const int font_tex_height = {0};", Img.Height);

			WriteLine("static const uint8_t font_tex[] PROGMEM = {");
			Write("    ");

			for (int y = 0; y < Img.Height; y++) {
				for (int x = 0; x < Img.Width; x++) {
					Rgba32 pixel = Img[x, y];

					Write("0x{0:X2}, 0x{1:X2}, 0x{2:X2}, ", pixel.R, pixel.G, pixel.B);
				}
			}

			WriteLine("");
			WriteLine("};");

			SW.Flush();
			SW.Close();
			SW.Dispose();
			FS.Dispose();

			Console.WriteLine("Done!");
			Console.ReadLine();
		}
	}
}

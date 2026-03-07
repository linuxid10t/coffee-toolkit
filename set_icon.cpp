// set_icon.cpp
// Post-build helper: scales toolkit.png to 32x32 and 16x16, converts to
// the BeOS CMAP8 palette, and writes BEOS:APP_ICON / BEOS:MINI_ICON
// attributes on the compiled binary so Tracker displays the icon.
//
// Usage: set_icon <binary> <icon.png>
// Build: g++ -o set_icon set_icon.cpp -lbe -lroot -ltranslation

#include <Application.h>
#include <Bitmap.h>
#include <NodeInfo.h>
#include <Screen.h>
#include <TranslationUtils.h>
#include <View.h>
#include <stdio.h>
#include <string.h>

// Nearest-neighbour downscale; both bitmaps must be B_RGBA32.
static void
scale_nearest(BBitmap* dst, const BBitmap* src)
{
	BRect sb = src->Bounds(), db = dst->Bounds();
	int sw = (int)sb.Width() + 1, sh = (int)sb.Height() + 1;
	int dw = (int)db.Width() + 1, dh = (int)db.Height() + 1;
	const uint8* sp = (const uint8*)src->Bits();
	uint8* dp = (uint8*)dst->Bits();
	int sBPR = src->BytesPerRow();
	int dBPR = dst->BytesPerRow();

	for (int y = 0; y < dh; y++) {
		int sy = y * sh / dh;
		for (int x = 0; x < dw; x++) {
			int sx = x * sw / dw;
			memcpy(dp + y * dBPR + x * 4, sp + sy * sBPR + sx * 4, 4);
		}
	}
}

// Returns a B_RGBA32 copy of src (caller owns it). Returns src unchanged
// if it is already B_RGBA32 (caller must NOT double-delete in that case).
static BBitmap*
ensure_rgba32(BBitmap* src)
{
	if (src->ColorSpace() == B_RGBA32)
		return src;

	BBitmap* out = new BBitmap(src->Bounds(), B_RGBA32, true);
	BView* v = new BView(src->Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW);
	out->AddChild(v);
	out->Lock();
	v->DrawBitmap(src, src->Bounds());
	v->Sync();
	out->Unlock();
	return out;
}

class IconSetterApp : public BApplication {
public:
	IconSetterApp(const char* binary, const char* png)
		: BApplication("application/x-vnd.IconSetter"),
		  fBinary(binary), fPNG(png), fResult(0)
	{}

	void ReadyToRun() override
	{
		fResult = DoIt();
		PostMessage(B_QUIT_REQUESTED);
	}

	int Result() const { return fResult; }

private:
	int DoIt()
	{
		BBitmap* raw = BTranslationUtils::GetBitmap(fPNG);
		if (raw == NULL) {
			fprintf(stderr, "set_icon: cannot load '%s'\n", fPNG);
			return 1;
		}

		BBitmap* rgba = ensure_rgba32(raw);

		BNode node(fBinary);
		if (node.InitCheck() != B_OK) {
			fprintf(stderr, "set_icon: cannot open '%s'\n", fBinary);
			if (rgba != raw) delete rgba;
			delete raw;
			return 1;
		}

		BNodeInfo info(&node);
		BScreen screen(B_MAIN_SCREEN_ID);
		if (!screen.IsValid()) {
			fprintf(stderr, "set_icon: no screen — cannot map palette\n");
			if (rgba != raw) delete rgba;
			delete raw;
			return 1;
		}

		SetIcon(rgba, &info, &screen, B_LARGE_ICON, 32);
		SetIcon(rgba, &info, &screen, B_MINI_ICON,  16);

		if (rgba != raw) delete rgba;
		delete raw;

		printf("set_icon: applied icon to '%s'\n", fBinary);
		return 0;
	}

	void SetIcon(BBitmap* rgba, BNodeInfo* info, BScreen* screen,
	             icon_size sz, int dim)
	{
		// Scale to target size in RGBA32
		BBitmap* scaled = new BBitmap(BRect(0, 0, dim - 1, dim - 1), B_RGBA32);
		scale_nearest(scaled, rgba);

		// Convert to CMAP8 (what BNodeInfo::SetIcon expects)
		BBitmap* cmap = new BBitmap(BRect(0, 0, dim - 1, dim - 1), B_CMAP8);
		const uint8* sp = (const uint8*)scaled->Bits();
		uint8* dp = (uint8*)cmap->Bits();
		int sBPR = scaled->BytesPerRow();
		int dBPR = cmap->BytesPerRow();

		for (int y = 0; y < dim; y++) {
			for (int x = 0; x < dim; x++) {
				// Haiku B_RGBA32 byte order: B G R A
				const uint8* p = sp + y * sBPR + x * 4;
				rgb_color c = { p[2], p[1], p[0], 255 };
				dp[y * dBPR + x] = screen->IndexForColor(c);
			}
		}

		info->SetIcon(cmap, sz);
		delete scaled;
		delete cmap;
	}

	const char* fBinary;
	const char* fPNG;
	int fResult;
};

int
main(int argc, char** argv)
{
	if (argc < 3) {
		fprintf(stderr, "Usage: set_icon <binary> <icon.png>\n");
		return 1;
	}
	IconSetterApp app(argv[1], argv[2]);
	app.Run();
	return app.Result();
}

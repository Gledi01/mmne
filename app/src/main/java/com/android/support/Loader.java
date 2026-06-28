package com.android.support;

import android.animation.*;
import android.content.*;
import android.graphics.*;
import android.graphics.drawable.*;
import android.util.*;
import android.os.*;
import android.view.*;
import android.text.*;
import android.widget.*;
import com.android.support.Menu;
import android.app.AlertDialog;
import android.net.Uri;
import java.util.ArrayList;

public class Loader 
{
	protected static Context context;
	protected LinearLayout childOfScroll;
	
	public static native void Changes(int feature, int value);
    native String[] GetFeatureList();
	
	public static boolean hide;
	public static boolean close;
	
    native String Icon();
    native String setTitleText();
    native String setHeadingText();
	public static native void initNativeContext(Context context);
	
	public static void showToastFromNative(final Context ctx, final String msg) {
		android.os.Handler handler = new android.os.Handler(android.os.Looper.getMainLooper());
		handler.post(new Runnable() {
			public void run() {
				android.widget.Toast.makeText(ctx, msg, android.widget.Toast.LENGTH_SHORT).show();
			}
		});
	}

	public static void Start(final Context context)
	{
        System.loadLibrary("DarkTeam");
        Handler handler = new Handler();
   		handler.postDelayed(new Runnable() {
                @Override
                public void run() {
					new Loader().initFloating(context);
                }
            }, 3000);
	    } 
      
	public final void initFloating(final Context context)
	{      
		Loader.context = context;
		initNativeContext(context);

		Menu menu = new Menu(context);
		menu.setWidth(menu.dpi(310));
		menu.setHeight(menu.dpi(420));
		menu.setIconImage(Icon());
		menu.setTitle(setTitleText());

		String[] listFT = GetFeatureList();

		// ── Parse kategori ──
		// Kelompokkan fitur per kategori
		ArrayList<String> categoryNames = new ArrayList<>();
		ArrayList<ArrayList<Integer>> categoryIndices = new ArrayList<>();
		ArrayList<Integer> currentGroup = null;

		for (int i = 0; i < listFT.length; i++) {
			String str = listFT[i];
			if (str.startsWith("Category_")) {
				categoryNames.add(str.substring(9));
				currentGroup = new ArrayList<>();
				categoryIndices.add(currentGroup);
			} else {
				if (currentGroup == null) {
					// Fitur sebelum kategori pertama — buat grup default
					categoryNames.add("Menu");
					currentGroup = new ArrayList<>();
					categoryIndices.add(currentGroup);
				}
				currentGroup.add(i);
			}
		}

		// ── Tab bar kategori ──
		final LinearLayout tabBar = new LinearLayout(context);
		tabBar.setOrientation(LinearLayout.HORIZONTAL);
		tabBar.setPadding(8, 4, 8, 4);
		LinearLayout.LayoutParams tabBarLp = new LinearLayout.LayoutParams(-1, -2);
		tabBar.setLayoutParams(tabBarLp);
		menu.getChildOfScroll().addView(tabBar);

		// Divider bawah tab
		View tabDivider = new View(context);
		tabDivider.setBackgroundColor(Color.parseColor("#3300FFFF"));
		LinearLayout.LayoutParams divLp = new LinearLayout.LayoutParams(-1, 1);
		divLp.setMargins(8, 2, 8, 6);
		menu.getChildOfScroll().addView(tabDivider, divLp);

		// ── Content container ──
		final LinearLayout contentContainer = new LinearLayout(context);
		contentContainer.setOrientation(LinearLayout.VERTICAL);
		contentContainer.setLayoutParams(new LinearLayout.LayoutParams(-1, -2));
		menu.getChildOfScroll().addView(contentContainer);

		// Build semua panel konten (tersembunyi dulu)
		final LinearLayout[] panels = new LinearLayout[categoryNames.size()];
		for (int c = 0; c < categoryNames.size(); c++) {
			LinearLayout panel = new LinearLayout(context);
			panel.setOrientation(LinearLayout.VERTICAL);
			panel.setLayoutParams(new LinearLayout.LayoutParams(-1, -2));
			panel.setVisibility(View.GONE);
			contentContainer.addView(panel);
			panels[c] = panel;

			// Isi panel dengan widget
			ArrayList<Integer> indices = categoryIndices.get(c);
			for (int idx : indices) {
				final int feature = idx;
				String str = listFT[idx];
				String[] split = str.split("_");

				if (str.startsWith("SeekBar_")) {
					// Format: SeekBar_NamaFitur_defaultVal_maxVal
					String fname = split[1];
					int defVal = Integer.parseInt(split[2]);
					int maxVal = Integer.parseInt(split[3]);
					addSeekBarToPanel(context, panel, feature, fname, defVal, maxVal);

				} else if (str.startsWith("ButtonOnOff_")) {
					addButtonOnOffToPanel(context, panel, feature, split[1], menu);

				} else if (str.startsWith("Button_")) {
					addButtonToPanel(context, panel, feature, split[1], menu);

				} else if (str.startsWith("WhatsApp_")) {
					final String waNumber = split[1];
					final String waLabel = split[2];
					addWaButtonToPanel(context, panel, waNumber, waLabel, menu);

				} else if (str.startsWith("Hide_")) {
					addHideButtonToPanel(context, panel, split[1], menu);

				} else if (str.startsWith("Close_")) {
					addCloseButtonToPanel(context, panel, split[1], menu);

				} else if (str.startsWith("Text_")) {
					TextView tv = new TextView(context);
					tv.setText(Html.fromHtml("<b>" + str.substring(5) + "</b>"));
					tv.setTextColor(Color.parseColor("#00FFFF"));
					tv.setTextSize(12f);
					tv.setPadding(20, 4, 0, 4);
					tv.setLayoutParams(new LinearLayout.LayoutParams(-1, -2));
					panel.addView(tv);
				}
			}
		}

		// ── Tab buttons ──
		final Button[] tabBtns = new Button[categoryNames.size()];
		for (int c = 0; c < categoryNames.size(); c++) {
			final int catIdx = c;
			Button tab = new Button(context);
			LinearLayout.LayoutParams tlp = new LinearLayout.LayoutParams(0, menu.dpi(34), 1f);
			tlp.setMargins(3, 0, 3, 0);
			tab.setLayoutParams(tlp);
			tab.setText(categoryNames.get(c));
			tab.setTextSize(10f);
			tab.setAllCaps(false);
			tab.setLetterSpacing(0.02f);
			tab.setTextColor(Color.WHITE);
			tab.setBackground(makeTabBg(context, false));
			tab.setOnClickListener(new View.OnClickListener() {
				public void onClick(View v) {
					// Sembunyikan semua panel
					for (LinearLayout p : panels) p.setVisibility(View.GONE);
					for (Button b : tabBtns) b.setBackground(makeTabBg(context, false));
					for (Button b : tabBtns) b.setTextColor(Color.WHITE);
					// Tampilkan panel yang dipilih
					panels[catIdx].setVisibility(View.VISIBLE);
					tab.setBackground(makeTabBg(context, true));
					tab.setTextColor(Color.parseColor("#00FFFF"));
				}
			});
			tabBar.addView(tab);
			tabBtns[c] = tab;
		}

		// Aktifkan tab pertama
		if (panels.length > 0) {
			panels[0].setVisibility(View.VISIBLE);
			tabBtns[0].setBackground(makeTabBg(context, true));
			tabBtns[0].setTextColor(Color.parseColor("#00FFFF"));
		}
	}

	// ── Helper: Tab background ──
	private GradientDrawable makeTabBg(Context ctx, boolean active) {
		GradientDrawable gd = new GradientDrawable();
		gd.setShape(GradientDrawable.RECTANGLE);
		gd.setCornerRadius(10f);
		if (active) {
			gd.setColor(Color.parseColor("#2200FFFF"));
			gd.setStroke(2, Color.parseColor("#00FFFF"));
		} else {
			gd.setColor(Color.parseColor("#11FFFFFF"));
			gd.setStroke(1, Color.parseColor("#3300FFFF"));
		}
		return gd;
	}

	private GradientDrawable makeButtonBg(Context ctx, boolean filled) {
		GradientDrawable gd = new GradientDrawable();
		gd.setShape(GradientDrawable.RECTANGLE);
		gd.setCornerRadius(12f);
		if (filled) {
			gd.setColor(Color.parseColor("#2200FFFF"));
			gd.setStroke(2, Color.parseColor("#00FFFF"));
		} else {
			gd.setColor(Color.parseColor("#11FFFFFF"));
			gd.setStroke(2, Color.parseColor("#4400FFFF"));
		}
		return gd;
	}

	// ── Widget helpers ──
	private void addSeekBarToPanel(Context ctx, LinearLayout panel, final int feature,
	                                String fname, int defVal, int maxVal) {
		LinearLayout ll = new LinearLayout(ctx);
		ll.setOrientation(LinearLayout.VERTICAL);
		ll.setPadding(10, 5, 10, 5);
		ll.setLayoutParams(new LinearLayout.LayoutParams(-1, -2));

		final TextView label = new TextView(ctx);
		label.setText(fname + " : " + defVal);
		label.setTextSize(13f);
		label.setTextColor(Color.WHITE);
		label.setPadding(5, 0, 0, 0);
		label.setLayoutParams(new LinearLayout.LayoutParams(-1, -2));

		SeekBar sb = new SeekBar(ctx);
		sb.setMax(maxVal);
		sb.setProgress(defVal);
		sb.getProgressDrawable().setColorFilter(Color.parseColor("#00FFFF"), PorterDuff.Mode.MULTIPLY);
		sb.getThumb().setColorFilter(Color.parseColor("#00FFFF"), PorterDuff.Mode.MULTIPLY);
		sb.setPadding(25, 10, 35, 10);
		LinearLayout.LayoutParams sblp = new LinearLayout.LayoutParams(-1, -2);
		sblp.bottomMargin = 10;
		sb.setLayoutParams(sblp);

		sb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
			public void onStartTrackingTouch(SeekBar s) {}
			public void onStopTrackingTouch(SeekBar s) {}
			public void onProgressChanged(SeekBar s, int i, boolean z) {
				label.setText(fname + " : " + i);
				Changes(feature, i);
			}
		});

		ll.addView(label);
		ll.addView(sb);
		panel.addView(ll);
	}

	private void addButtonToPanel(Context ctx, LinearLayout panel, final int feature, final String fname, final Menu menu) {
		final Button btn = new Button(ctx);
		LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, menu.dpi(42));
		lp.setMargins(10, 4, 10, 4);
		btn.setLayoutParams(lp);
		btn.setText(fname);
		btn.setTextColor(Color.WHITE);
		btn.setTextSize(14f);
		btn.setAllCaps(false);
		btn.setLetterSpacing(0.05f);
		btn.setBackground(makeButtonBg(ctx, false));
		btn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				btn.setBackground(makeButtonBg(ctx, true));
				btn.setTextColor(Color.parseColor("#00FFFF"));
				btn.setText(Html.fromHtml("✦ <b>" + fname + "</b>"));
				new Handler().postDelayed(new Runnable() {
					public void run() {
						btn.setBackground(makeButtonBg(ctx, false));
						btn.setText(fname);
						btn.setTextColor(Color.WHITE);
					}
				}, 120);
				Changes(feature, 0);
			}
		});
		panel.addView(btn);
	}

	private void addButtonOnOffToPanel(Context ctx, LinearLayout panel, final int feature, final String fname, final Menu menu) {
		final Button btn = new Button(ctx);
		LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, menu.dpi(42));
		lp.setMargins(10, 4, 10, 4);
		btn.setLayoutParams(lp);
		btn.setText(fname);
		btn.setTextColor(Color.WHITE);
		btn.setTextSize(14f);
		btn.setAllCaps(false);
		btn.setBackground(makeButtonBg(ctx, false));
		btn.setOnClickListener(new View.OnClickListener() {
			private boolean isActive = true;
			public void onClick(View v) {
				Changes(feature, 0);
				if (isActive) {
					btn.setText(Html.fromHtml("✦ <b>" + fname + "</b> ✦"));
					btn.setBackground(makeButtonBg(ctx, true));
					btn.setTextColor(Color.parseColor("#00FFFF"));
					isActive = false;
				} else {
					btn.setText(fname);
					btn.setBackground(makeButtonBg(ctx, false));
					btn.setTextColor(Color.WHITE);
					isActive = true;
				}
			}
		});
		panel.addView(btn);
	}

	private void addWaButtonToPanel(Context ctx, LinearLayout panel, final String number, String label, Menu menu) {
		final Button btn = new Button(ctx);
		LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, menu.dpi(42));
		lp.setMargins(10, 4, 10, 4);
		btn.setLayoutParams(lp);
		btn.setText("💬 " + label);
		btn.setTextColor(Color.parseColor("#25D366"));
		btn.setTextSize(14f);
		btn.setAllCaps(false);
		GradientDrawable gd = new GradientDrawable();
		gd.setShape(GradientDrawable.RECTANGLE);
		gd.setCornerRadius(12f);
		gd.setColor(Color.parseColor("#1125D366"));
		gd.setStroke(2, Color.parseColor("#25D366"));
		btn.setBackground(gd);
		btn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				try {
					Intent intent = new Intent(Intent.ACTION_VIEW);
					intent.setData(Uri.parse("https://wa.me/" + number));
					intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
					context.startActivity(intent);
				} catch (Exception e) { e.printStackTrace(); }
			}
		});
		panel.addView(btn);
	}

	private void addHideButtonToPanel(Context ctx, LinearLayout panel, final String fname, final Menu menu) {
		final Button btn = new Button(ctx);
		LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, menu.dpi(42));
		lp.setMargins(10, 4, 10, 4);
		btn.setLayoutParams(lp);
		btn.setText(fname);
		btn.setTextColor(Color.WHITE);
		btn.setTextSize(14f);
		btn.setAllCaps(false);
		btn.setBackground(makeButtonBg(ctx, false));
		btn.setOnClickListener(new View.OnClickListener() {
			private boolean isActive = true;
			public void onClick(View v) {
				hide = !hide;
				if (isActive) {
					btn.setText(Html.fromHtml("✦ <b>" + fname + "</b> ✦"));
					btn.setBackground(makeButtonBg(ctx, true));
					btn.setTextColor(Color.parseColor("#00FFFF"));
					isActive = false;
				} else {
					btn.setText(fname);
					btn.setBackground(makeButtonBg(ctx, false));
					btn.setTextColor(Color.WHITE);
					isActive = true;
				}
			}
		});
		panel.addView(btn);
	}

	private void addCloseButtonToPanel(Context ctx, LinearLayout panel, final String fname, final Menu menu) {
		final Button btn = new Button(ctx);
		LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, menu.dpi(42));
		lp.setMargins(10, 4, 10, 8);
		btn.setLayoutParams(lp);
		btn.setText("✕  " + fname);
		btn.setTextColor(Color.parseColor("#FF4444"));
		btn.setTextSize(14f);
		btn.setAllCaps(false);
		GradientDrawable gd = new GradientDrawable();
		gd.setShape(GradientDrawable.RECTANGLE);
		gd.setCornerRadius(12f);
		gd.setColor(Color.parseColor("#22FF0000"));
		gd.setStroke(2, Color.parseColor("#FF4444"));
		btn.setBackground(gd);
		btn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				close = !close;
				new Handler().postDelayed(new Runnable() {
					public void run() { menu.showIcon(); }
				}, 120);
			}
		});
		panel.addView(btn);
	}
				}
			

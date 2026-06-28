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
import android.R;
import android.net.Uri;
import java.util.regex.MatchResult;
import java.io.UnsupportedEncodingException;

public class Loader 
{
	protected static Context context;
	protected LinearLayout childOfScroll;
	protected LinearLayout currentCategoryLayout;
	protected LinearLayout statsLayout, itemsLayout, lootLayout, backupLayout;
	
	public static native void Changes(int feature, int value);
    native String[] GetFeatureList();
	
	public static boolean hide;
	public static boolean close;
	
    native String Icon();
    native String setTitleText();
    native String setHeadingText();

	public static native void initNativeContext(Context context);
	
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
      
	private void showCategory(LinearLayout layout) {
		childOfScroll.removeAllViews();
		childOfScroll.addView(layout);
	}

	private void createCategoryButtons(Menu menu) {
		LinearLayout categoryBar = new LinearLayout(menu.context);
		categoryBar.setOrientation(LinearLayout.HORIZONTAL);
		categoryBar.setPadding(4, 8, 4, 4);

		String[] categories = {"Stats", "Items", "Loot", "Backup"};
		final LinearLayout[] layouts = {statsLayout, itemsLayout, lootLayout, backupLayout};

		for (int i = 0; i < categories.length; i++) {
			final int idx = i;
			Button btn = new Button(menu.context);
			LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(0, menu.dpi(36), 1);
			lp.setMargins(2, 0, 2, 0);
			btn.setText(categories[i]);
			btn.setTextSize(11f);
			btn.setAllCaps(false);
			btn.setBackground(menu.makeButtonBg(false));
			btn.setTextColor(Color.WHITE);
			btn.setLayoutParams(lp);
			btn.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					showCategory(layouts[idx]);
				}
			});
			categoryBar.addView(btn);
		}
		menu.getChildOfScroll().addView(categoryBar);
	}

	private void createStatsCategory(Menu menu) {
		statsLayout = new LinearLayout(menu.context);
		statsLayout.setOrientation(LinearLayout.VERTICAL);
		statsLayout.setPadding(10, 10, 10, 10);

		// EXP
		addInputSection(statsLayout, menu, "Set EXP", "exp_input", 0);
		// Level
		addInputSection(statsLayout, menu, "Set Level", "level_input", 1);
		// Skill Points
		addInputSection(statsLayout, menu, "Skill Points", "skill_input", 2);
	}

	private void createItemsCategory(Menu menu) {
		itemsLayout = new LinearLayout(menu.context);
		itemsLayout.setOrientation(LinearLayout.VERTICAL);
		itemsLayout.setPadding(10, 10, 10, 10);

		// Item Name
		LinearLayout row1 = new LinearLayout(menu.context);
		row1.setOrientation(LinearLayout.HORIZONTAL);
		row1.setPadding(0, 4, 0, 4);

		TextView tv1 = new TextView(menu.context);
		tv1.setText("Item:");
		tv1.setTextColor(Color.parseColor("#00FFFF"));
		tv1.setTextSize(12f);
		tv1.setLayoutParams(new LinearLayout.LayoutParams(menu.dpi(60), -2));

		EditText itemName = new EditText(menu.context);
		itemName.setHint("coin, gem, wood...");
		itemName.setHintTextColor(Color.parseColor("#4400FFFF"));
		itemName.setTextColor(Color.WHITE);
		itemName.setBackgroundColor(Color.parseColor("#220A0F1A"));
		itemName.setTag("item_name");
		LinearLayout.LayoutParams lp1 = new LinearLayout.LayoutParams(0, menu.dpi(36), 1);
		lp1.setMargins(8, 0, 0, 0);
		itemName.setLayoutParams(lp1);
		row1.addView(tv1);
		row1.addView(itemName);
		itemsLayout.addView(row1);

		// Count
		addInputSection(itemsLayout, menu, "Count", "item_count", 3);

		// Execute Button
		Button execBtn = new Button(menu.context);
		execBtn.setText("✦ Give Item");
		execBtn.setTextColor(Color.parseColor("#00FFFF"));
		execBtn.setBackground(menu.makeButtonBg(true));
		LinearLayout.LayoutParams execLp = new LinearLayout.LayoutParams(-1, menu.dpi(42));
		execLp.setMargins(0, 12, 0, 0);
		execBtn.setLayoutParams(execLp);
		execBtn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				EditText nameField = (EditText) itemsLayout.findViewWithTag("item_name");
				EditText countField = (EditText) itemsLayout.findViewWithTag("item_count");
				if (nameField != null && countField != null) {
					String name = nameField.getText().toString().trim();
					try {
						int count = Integer.parseInt(countField.getText().toString());
						callGiveItem(name, count);
					} catch (Exception e) {
						Toast.makeText(menu.context, "Invalid count!", Toast.LENGTH_SHORT).show();
					}
				}
			}
		});
		itemsLayout.addView(execBtn);
	}

	private void createLootCategory(Menu menu) {
		lootLayout = new LinearLayout(menu.context);
		lootLayout.setOrientation(LinearLayout.VERTICAL);
		lootLayout.setPadding(10, 10, 10, 10);

		Button lootBtn = new Button(menu.context);
		lootBtn.setText("🎲 Generate Chest Loot");
		lootBtn.setTextColor(Color.WHITE);
		lootBtn.setBackground(menu.makeButtonBg(false));
		LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, menu.dpi(48));
		lootBtn.setLayoutParams(lp);
		lootBtn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				callGenerateLoot();
				Toast.makeText(menu.context, "Loot generated!", Toast.LENGTH_SHORT).show();
			}
		});
		lootLayout.addView(lootBtn);
	}

	private void createBackupCategory(Menu menu) {
		backupLayout = new LinearLayout(menu.context);
		backupLayout.setOrientation(LinearLayout.VERTICAL);
		backupLayout.setPadding(10, 10, 10, 10);

		Button backupBtn = new Button(menu.context);
		backupBtn.setText("💾 Backup Inventory");
		backupBtn.setTextColor(Color.WHITE);
		backupBtn.setBackground(menu.makeButtonBg(false));
		LinearLayout.LayoutParams lp1 = new LinearLayout.LayoutParams(-1, menu.dpi(42));
		lp1.setMargins(0, 0, 0, 8);
		backupBtn.setLayoutParams(lp1);
		backupBtn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Changes(100, 0); // Feature 100 = backup
			}
		});
		backupLayout.addView(backupBtn);

		Button restoreBtn = new Button(menu.context);
		restoreBtn.setText("📥 Restore Inventory");
		restoreBtn.setTextColor(Color.WHITE);
		restoreBtn.setBackground(menu.makeButtonBg(false));
		LinearLayout.LayoutParams lp2 = new LinearLayout.LayoutParams(-1, menu.dpi(42));
		restoreBtn.setLayoutParams(lp2);
		restoreBtn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Changes(101, 0); // Feature 101 = restore
			}
		});
		backupLayout.addView(restoreBtn);
	}

	private void addInputSection(LinearLayout parent, Menu menu, String label, String tag, final int feature) {
		LinearLayout row = new LinearLayout(menu.context);
		row.setOrientation(LinearLayout.HORIZONTAL);
		row.setPadding(0, 4, 0, 4);

		TextView tv = new TextView(menu.context);
		tv.setText(label + ":");
		tv.setTextColor(Color.parseColor("#00FFFF"));
		tv.setTextSize(12f);
		tv.setLayoutParams(new LinearLayout.LayoutParams(menu.dpi(90), -2));

		EditText input = new EditText(menu.context);
		input.setInputType(android.text.InputType.TYPE_CLASS_NUMBER);
		input.setHint("0");
		input.setHintTextColor(Color.parseColor("#4400FFFF"));
		input.setTextColor(Color.WHITE);
		input.setBackgroundColor(Color.parseColor("#220A0F1A"));
		input.setTag(tag);
		LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(0, menu.dpi(36), 1);
		lp.setMargins(8, 0, 0, 0);
		input.setLayoutParams(lp);

		Button execBtn = new Button(menu.context);
		execBtn.setText("✓");
		execBtn.setTextColor(Color.parseColor("#00FF00"));
		execBtn.setBackground(menu.makeButtonBg(true));
		LinearLayout.LayoutParams btnLp = new LinearLayout.LayoutParams(menu.dpi(36), menu.dpi(36));
		btnLp.setMargins(8, 0, 0, 0);
		execBtn.setLayoutParams(btnLp);

		final int finalFeature = feature;
		final EditText finalInput = input;
		execBtn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				try {
					int value = Integer.parseInt(finalInput.getText().toString());
					Changes(finalFeature, value);
					Toast.makeText(menu.context, "Executed!", Toast.LENGTH_SHORT).show();
				} catch (Exception e) {
					Toast.makeText(menu.context, "Invalid input!", Toast.LENGTH_SHORT).show();
				}
			}
		});

		row.addView(tv);
		row.addView(input);
		row.addView(execBtn);
		parent.addView(row);
	}

	private void callGiveItem(String itemName, int count) {
		Changes(3, count); // Pass count di value, nama itemnya via native
	}

	private void callGenerateLoot() {
		Changes(4, 0); // Feature 4 = generate loot
	}

	public final void initFloating(final Context context)
	{      
		Loader.context = context;
		initNativeContext(context);

		Menu menu = new Menu(context);
		menu.setWidth(menu.dpi(320));
		menu.setHeight(menu.dpi(400));
		menu.setIconImage(Icon());
		menu.setTitle(setTitleText());
        
        TextView Title = new TextView(context);
        Title.setText(Html.fromHtml(setHeadingText()));
        Title.setTextColor(Color.WHITE);
        Title.setTextSize(13.5f);
        Title.setGravity(Gravity.CENTER);
		menu.getChildOfScroll().addView(Title);

		// Store reference
		childOfScroll = menu.getChildOfScroll();

		// Create all categories
		createStatsCategory(menu);
		createItemsCategory(menu);
		createLootCategory(menu);
		createBackupCategory(menu);

		// Create category buttons
		createCategoryButtons(menu);

		// Show stats by default
		showCategory(statsLayout);

		// Add WA button at bottom
		Button waBtn = new Button(context);
		waBtn.setText("💬 Chat Owner");
		waBtn.setTextColor(Color.parseColor("#25D366"));
		GradientDrawable gdWA = new GradientDrawable();
		gdWA.setShape(GradientDrawable.RECTANGLE);
		gdWA.setCornerRadius(12.0f);
		gdWA.setColor(Color.parseColor("#1125D366"));
		gdWA.setStroke(2, Color.parseColor("#25D366"));
		LinearLayout.LayoutParams waLp = new LinearLayout.LayoutParams(-1, menu.dpi(36));
		waLp.setMargins(10, 12, 10, 0);
		waBtn.setLayoutParams(waLp);
		waBtn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				try {
					Intent intent = new Intent(Intent.ACTION_VIEW);
					intent.setData(Uri.parse("https://wa.me/62xxxxxxxx")); // Ganti nomor lo
					intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
					context.startActivity(intent);
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		});
		menu.getChildOfScroll().addView(waBtn);
	}
  }
      

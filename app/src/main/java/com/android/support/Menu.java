package com.android.support;

import android.animation.*;
import android.app.*;
import android.content.*;
import android.content.res.*;
import android.graphics.*;
import android.graphics.drawable.*;
import android.os.*;
import com.android.support.*;
import android.text.*;
import android.view.Window.*; 
import android.text.method.*;
import android.graphics.Typeface;
import android.view.MotionEvent;
import android.util.*;
import android.view.*;
import android.widget.*;
import android.view.animation.AlphaAnimation; 
import android.view.animation.Animation;
import java.io.*;
import java.util.Objects;

public class Menu
{
	protected int WIDTH,HEIGHT;   
	protected Context context;
	protected boolean isIconVisible;
	protected boolean isMenuVisible;
	protected ImageView iconView;
	protected FrameLayout parentBox;
	protected LinearLayout menulayout;  
	protected ScrollView scrollItems;    
	
	protected TextView title;

	protected WindowManager mWindowManager;
	protected WindowManager.LayoutParams params;
	protected LinearLayout headerLayout;

	protected LinearLayout childOfScroll;
	protected LinearLayout infos;
	
	static int dur; 
	public static int one; 
	public static int zero;
	static { 
		dur = 400; 
		zero = 0; 
		one = 1; 
	} 

	public static Animation fadein() {
		AlphaAnimation alphaAnimation = new AlphaAnimation((float)one, (float)zero); 
		alphaAnimation.setDuration((long)dur); 
		return alphaAnimation; 
	} 

	public static Animation fadeout() {
		AlphaAnimation alphaAnimation = new AlphaAnimation((float)zero, (float)one); 
		alphaAnimation.setDuration((long)dur); 
		return alphaAnimation; 
	}

	// Neon color cycle
	private static final int[] NEON_COLORS = {
		0xFF00FFFF, // cyan
		0xFF0080FF, // blue
		0xFF8000FF, // purple
		0xFFFF00FF, // magenta
		0xFFFF0080, // pink
		0xFFFF4000, // orange-red
		0xFF00FF80, // green-cyan
		0xFF00FFFF  // back to cyan
	};

	private GradientDrawable gdMenuBody;
	private ValueAnimator neonAnim;

	private void startNeonBorder() {
		neonAnim = ValueAnimator.ofFloat(0f, 1f);
		neonAnim.setDuration(3000);
		neonAnim.setRepeatCount(ValueAnimator.INFINITE);
		neonAnim.setRepeatMode(ValueAnimator.RESTART);
		neonAnim.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
			@Override
			public void onAnimationUpdate(ValueAnimator anim) {
				float fraction = (float) anim.getAnimatedValue();
				int len = NEON_COLORS.length - 1;
				float scaled = fraction * len;
				int idx = (int) scaled;
				float f = scaled - idx;
				int c1 = NEON_COLORS[Math.min(idx, len - 1)];
				int c2 = NEON_COLORS[Math.min(idx + 1, len)];
				int color = blendColors(c1, c2, f);
				gdMenuBody.setStroke(4, color);
				title.setTextColor(color);
			}
		});
		neonAnim.start();
	}

	private int blendColors(int c1, int c2, float ratio) {
		float inv = 1f - ratio;
		int a = (int)((Color.alpha(c1) * inv) + (Color.alpha(c2) * ratio));
		int r = (int)((Color.red(c1) * inv) + (Color.red(c2) * ratio));
		int g = (int)((Color.green(c1) * inv) + (Color.green(c2) * ratio));
		int b = (int)((Color.blue(c1) * inv) + (Color.blue(c2) * ratio));
		return Color.argb(a, r, g, b);
	}

	public GradientDrawable makeButtonBg(boolean filled) {
		GradientDrawable gd = new GradientDrawable();
		gd.setShape(GradientDrawable.RECTANGLE);
		gd.setCornerRadius(12.0f);
		if (filled) {
			gd.setColor(Color.parseColor("#2200FFFF"));
			gd.setStroke(2, Color.parseColor("#00FFFF"));
		} else {
			gd.setColor(Color.parseColor("#11FFFFFF"));
			gd.setStroke(2, Color.parseColor("#4400FFFF"));
		}
		return gd;
	}

	public TextView addText(String text) {
        TextView textView = new TextView(context);
        textView.setText(Html.fromHtml("<b>" + text + "</b>"));
        textView.setTextColor(Color.parseColor("#00FFFF"));
        textView.setTextSize(12.0f);
        textView.setGravity(3);
        textView.setLetterSpacing(0.1f);
        textView.setLayoutParams(new LinearLayout.LayoutParams(-1, -2));
        textView.setPadding(20, 4, 0, 4);
        getChildOfScroll().addView(textView);
        return textView;
    }
    
	public void SeekBar(final int featNum, final String featName, final int prog, int max, final iit interInt) {
		LinearLayout linearLayout = new LinearLayout(context);
		linearLayout.setPadding(10, 5, 10, 5);
		linearLayout.setOrientation(LinearLayout.VERTICAL);
		linearLayout.setLayoutParams(new LinearLayout.LayoutParams(-1, -2));

		final TextView textView = new TextView(context);				
		textView.setText(featName + " : " + prog);
		textView.setTextSize(13.0f);
		textView.setGravity(3);
		textView.setTextColor(Color.WHITE);
		textView.setLayoutParams(new LinearLayout.LayoutParams(-1, -2));
		textView.setPadding(5, 0, 0, 0);

        SeekBar SeekBar = new SeekBar(context);
        SeekBar.setMax(max);
		SeekBar.getProgressDrawable().setColorFilter(Color.parseColor("#00FFFF"), PorterDuff.Mode.MULTIPLY);
        SeekBar.getThumb().setColorFilter(Color.parseColor("#00FFFF"), PorterDuff.Mode.MULTIPLY);
		SeekBar.setPadding(25, 10, 35, 10);
		LinearLayout.LayoutParams lp2 = new LinearLayout.LayoutParams(-1, -2);
		lp2.bottomMargin = 10;
		SeekBar.setLayoutParams(lp2);
		SeekBar.setProgress(prog);

		final TextView tv2 = textView;
		SeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
				public void onStartTrackingTouch(SeekBar seekBar) {}
				public void onStopTrackingTouch(SeekBar seekBar) {}
				int l;
				public void onProgressChanged(SeekBar seekBar, int i, boolean z) {
					l = i;
					interInt.OnWrite(i);
					tv2.setText(featName + " : " + i);
				}
			});
		linearLayout.addView(textView);
		linearLayout.addView(SeekBar);
		getChildOfScroll().addView(linearLayout);
	}
	
	public void ButtonOnOff(final int featNum, String featName, final ibt interfaceBtn) {
        final Button btn = new Button(context);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, dpi(42));
        lp.setMargins(10, 4, 10, 4);
        btn.setText(featName);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(14.0f);
        btn.setAllCaps(false);
        btn.setLetterSpacing(0.05f);
        btn.setBackground(makeButtonBg(false));
        btn.setLayoutParams(lp);
        final String fname = featName;
        btn.setOnClickListener(new View.OnClickListener() {
                private boolean isActive = true;              
                public void onClick(View v) {
                    interfaceBtn.OnWrite();
                    if (isActive) {
                        btn.setText(Html.fromHtml("✦ <b>" + fname + "</b> ✦"));
                        btn.setBackground(makeButtonBg(true));
                        btn.setTextColor(Color.parseColor("#00FFFF"));
                        isActive = false;
                    } else {
                        btn.setText(fname);
                        btn.setBackground(makeButtonBg(false));
                        btn.setTextColor(Color.WHITE);
                        isActive = true;
                    }
                }
            });
		getChildOfScroll().addView(btn);	
    }

	public void Button(final int featNum, String featName, final ibt interfaceBtn) {
        final Button btn = new Button(context);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, dpi(42));
        lp.setMargins(10, 4, 10, 4);
        btn.setText(featName);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(14.0f);
        btn.setAllCaps(false);
        btn.setLetterSpacing(0.05f);
        btn.setBackground(makeButtonBg(false));
        btn.setLayoutParams(lp);
        final String fname = featName;
        btn.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
					btn.setBackground(makeButtonBg(true));
					btn.setTextColor(Color.parseColor("#00FFFF"));
					btn.setText(Html.fromHtml("✦ <b>" + fname + "</b>"));
					final Handler handler = new Handler();
					handler.postDelayed(new Runnable() {
							@Override
							public void run() {
								btn.setBackground(makeButtonBg(false));
								btn.setText(fname);
								btn.setTextColor(Color.WHITE);
							}
						}, 120);
                    interfaceBtn.OnWrite();
                }  
            });
		getChildOfScroll().addView(btn);
    }	

	// Tombol WhatsApp khusus
	public void ButtonWA(String label, final ibt interfaceBtn) {
		final Button btn = new Button(context);
		LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, dpi(42));
		lp.setMargins(10, 4, 10, 4);
		btn.setText("💬 " + label);
		btn.setTextColor(Color.parseColor("#25D366"));
		btn.setTextSize(14.0f);
		btn.setAllCaps(false);
		btn.setLetterSpacing(0.05f);
		GradientDrawable gd = new GradientDrawable();
		gd.setShape(GradientDrawable.RECTANGLE);
		gd.setCornerRadius(12.0f);
		gd.setColor(Color.parseColor("#1125D366"));
		gd.setStroke(2, Color.parseColor("#25D366"));
		btn.setBackground(gd);
		btn.setLayoutParams(lp);
		btn.setOnClickListener(new View.OnClickListener() {
				public void onClick(View v) {
					interfaceBtn.OnWrite();
				}
			});
		getChildOfScroll().addView(btn);
	}
	
	public void CloseButton(final int featNum, String featName, final ibt interfaceBtn) {
        final Button btn = new Button(context);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, dpi(42));
        lp.setMargins(10, 4, 10, 8);
        btn.setText("✕  " + featName);
        btn.setTextColor(Color.parseColor("#FF4444"));
        btn.setTextSize(14.0f);
        btn.setAllCaps(false);
		GradientDrawable gd = new GradientDrawable();
		gd.setShape(GradientDrawable.RECTANGLE);
		gd.setCornerRadius(12.0f);
		gd.setColor(Color.parseColor("#22FF0000"));
		gd.setStroke(2, Color.parseColor("#FF4444"));
		btn.setBackground(gd);
        btn.setLayoutParams(lp);
        final String fname = featName;
        btn.setOnClickListener(new View.OnClickListener() {
                public void onClick(View view) {
					final Handler handler = new Handler();
					handler.postDelayed(new Runnable() {
							@Override
							public void run() {
								showIcon(); 
							}
						}, 120);
                    interfaceBtn.OnWrite();
                }  
            });
		getChildOfScroll().addView(btn);
    }	
	
	public LinearLayout getInfosLayout() { return infos; }
	public LinearLayout getChildOfScroll() { return childOfScroll; }
	public LinearLayout getHeaderLayout() { return headerLayout; }
	public LinearLayout getMenuLayout() { return menulayout; }
	public ScrollView getScrollView() { return scrollItems; }
	public TextView getTitleTextView() { return title; }
	public int getWidth(int px) { return WIDTH; }
	public int getHeight(int px) { return HEIGHT; }
	
	protected void init(Context context)
	{
		this.context = context;
		iconView = new ImageView(context);
		title = new TextView(context);

		parentBox = new FrameLayout(context);
		parentBox.setOnTouchListener(onTouchListener);
		
		int aditionalFlags = 0;
		if (Build.VERSION.SDK_INT >= 11)
			aditionalFlags = WindowManager.LayoutParams.FLAG_SPLIT_TOUCH;
		if (Build.VERSION.SDK_INT >= 3)
			aditionalFlags = aditionalFlags | WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM;
		params = new WindowManager.LayoutParams(
			WindowManager.LayoutParams.WRAP_CONTENT,
			WindowManager.LayoutParams.WRAP_CONTENT,		
			1,
			155,
			WindowManager.LayoutParams.TYPE_APPLICATION,
			WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE |
			WindowManager.LayoutParams.FLAG_LAYOUT_IN_OVERSCAN |
			WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN | aditionalFlags,
			PixelFormat.TRANSPARENT
		);
		params.gravity = Gravity.TOP | Gravity.LEFT;
	}

	public void setIconImage(String Icon)
	{
        byte[] decode = Base64.decode(Icon, 0);
        iconView.setImageBitmap(BitmapFactory.decodeByteArray(decode, 0, decode.length));
	    iconView.setPadding(dpi(7), dpi(7), 0, 0);
        iconView.setImageAlpha(200);
	}
	
	public void setWidth(int px) {
		FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams)menulayout.getLayoutParams();
		lp.width = px;
		menulayout.setLayoutParams(lp);
		WIDTH = px;
	}
	
	public void setHeight(int px) {
		FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams)menulayout.getLayoutParams();
		lp.height = px;
		menulayout.setLayoutParams(lp);
		HEIGHT = px;
	}
    
	public void setTitle(String text) {
		title.setText(text);	
		title.setTextSize(17);
		title.setTypeface(null, Typeface.BOLD);
		title.setLetterSpacing(0.15f);
		title.setGravity(Gravity.CENTER_HORIZONTAL);
	}

	public void showIcon() {
		if (Loader.hide) {
			iconView.setVisibility(View.INVISIBLE);
		} else {            
			iconView.setVisibility(View.VISIBLE);
		}
		if (!isIconVisible) {		
			parentBox.removeAllViews();
			parentBox.addView(iconView, dpi(70), dpi(70));	
            isMenuVisible = false;
			isIconVisible = true;
		}
	}

	public void showMenu() {
		if (!isMenuVisible) {		
			parentBox.removeAllViews();
			parentBox.addView(menulayout, WIDTH, HEIGHT);
            isIconVisible = false;
			isMenuVisible = true;          	
	     }
      }
      
	public int dpi(float dp) {
		float scale = context.getResources().getDisplayMetrics().density;
		return (int)(dp * scale + 0.5f);
	}
	
	public Menu(Context context)
	{
		init(context);

		WIDTH = dpi(100);
		HEIGHT = dpi(50);

		// Background glassmorphism gelap
		gdMenuBody = new GradientDrawable();
		gdMenuBody.setColor(Color.parseColor("#CC0A0F1A"));
		gdMenuBody.setCornerRadius(24.0f);
		gdMenuBody.setStroke(4, Color.parseColor("#00FFFF"));

		// Start neon RGB border animation
		startNeonBorder();

		menulayout = new LinearLayout(context);
	    menulayout.setBackground(gdMenuBody);
		menulayout.setOrientation(LinearLayout.VERTICAL);
		menulayout.setPadding(0, 0, 0, 8);

		{
			headerLayout = new LinearLayout(context);
			headerLayout.setPadding(12, 10, 12, 8);

			// Garis bawah header
			GradientDrawable headerLine = new GradientDrawable(
				GradientDrawable.Orientation.LEFT_RIGHT,
				new int[]{Color.TRANSPARENT, Color.parseColor("#4400FFFF"), Color.TRANSPARENT}
			);

			menulayout.addView(headerLayout, -1, -2);
			{
				ImageView minimize = new ImageView(context);
				Bitmap bitmap = null;
				AssetManager assetManager = context.getAssets();
				try {
					InputStream istr = assetManager.open("null");
					bitmap = BitmapFactory.decodeStream(istr);
				} catch (IOException e) {
					e.printStackTrace();
				} finally {
					minimize.setImageBitmap(bitmap);
				}
				
				{
					infos = new LinearLayout(context);
					infos.setOrientation(LinearLayout.VERTICAL);
					infos.addView(title, -1, -1);
					headerLayout.addView(infos, -1, -1);
					LinearLayout.LayoutParams mnp = (LinearLayout.LayoutParams)infos.getLayoutParams();
					mnp.gravity = Gravity.CENTER;
					infos.setLayoutParams(mnp);
				}
				headerLayout.addView(minimize, dpi(35), dpi(35));
				{
					LinearLayout.LayoutParams mnp = (LinearLayout.LayoutParams)minimize.getLayoutParams();
					mnp.weight = 0;
					mnp.gravity = Gravity.RIGHT;
					minimize.setLayoutParams(mnp);
					minimize.setPadding(0, dpi(10), dpi(10), dpi(10));
				}

				// Divider neon tipis bawah header
				View divider = new View(context);
				divider.setBackgroundColor(Color.parseColor("#3300FFFF"));
				LinearLayout.LayoutParams divLp = new LinearLayout.LayoutParams(-1, 1);
				divLp.setMargins(16, 0, 16, 0);
				menulayout.addView(divider, divLp);
			}
		}
               
		scrollItems = new ScrollView(context);
		childOfScroll = new LinearLayout(context);
        scrollItems.setVerticalScrollBarEnabled(false);
        scrollItems.setOverScrollMode(View.OVER_SCROLL_NEVER);
		scrollItems.addView(childOfScroll, -1, -1);
        
		childOfScroll.setOrientation(LinearLayout.VERTICAL);
		childOfScroll.setBackgroundColor(Color.TRANSPARENT);
		childOfScroll.setPadding(0, 6, 0, 6);
        
		menulayout.addView(scrollItems, -1, -1);
		
	    showMenu();
        showIcon();
		
	    mWindowManager = ((Activity)context).getWindowManager();
		mWindowManager.addView(parentBox, params);
	}

	View.OnTouchListener onTouchListener = new View.OnTouchListener()
	{
		private float initialX;          
		private float initialY;
		private float initialTouchX;
		private float initialTouchY;
		double clock = 0;
		
		@Override
		public boolean onTouch(View view, MotionEvent motionEvent)
		{
			switch (motionEvent.getAction())
			{
				case MotionEvent.ACTION_DOWN:
					initialX = params.x;
					initialY = params.y;
					initialTouchX = motionEvent.getRawX();
					initialTouchY = motionEvent.getRawY();	
					clock = System.currentTimeMillis();
					break;

				case MotionEvent.ACTION_MOVE:
				    params.x = (int)initialX + (int)(motionEvent.getRawX() - initialTouchX);
					params.y = (int)initialY + (int)(motionEvent.getRawY() - initialTouchY);
					mWindowManager.updateViewLayout(parentBox, params);
					break;

				case MotionEvent.ACTION_UP:
					if (isIconVisible && (System.currentTimeMillis() < (clock + 200))) {
						showMenu();
					}
					break;
			}
			return true;
		}
	};

	public static interface ibt {
        void OnWrite();
    }
	
	public static interface iit {
        void OnWrite(int i);
    }
					}
				  

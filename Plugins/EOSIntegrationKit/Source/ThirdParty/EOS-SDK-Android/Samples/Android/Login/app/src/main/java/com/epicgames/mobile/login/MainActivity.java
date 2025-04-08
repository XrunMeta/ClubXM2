package com.epicgames.mobile.login;

import android.app.Activity;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkRequest;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.provider.Settings;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.NonNull;

import com.epicgames.mobile.eossdk.EOSSDK;

public class MainActivity extends Activity {
	static {
		System.loadLibrary("EOSSDK");
		System.loadLibrary("native-lib");
	}

	private static final String Tag = "EOSLoginSample";
	private TextView console;

	private LinearLayout loginLayout;
	private Button loginBtn;
	private ProgressBar loginSpinner;

	private LinearLayout logoutLayout;
	private Button logoutBtn;
	private ProgressBar logoutSpinner;

	private TextView username;

	private Handler handler = null;
	private Runnable r = null;

	private boolean bShowLoginSpinner = false;
	private boolean bLoggedIn = false;

	private boolean DataAvailable = false;
	private boolean WifiAvailable = false;

	ConnectivityManager connectivityManager = null;
	TelephonyManager telephonyManager = null;
	ConnectivityManager.NetworkCallback networkCallback = null;
	PhoneStateListener phoneStateListener = null;

	/**
	 * This will maintain calling itself every 10th of a second after the initial trigger
	 */
	void startGameLoop() {
		handler = new Handler();
		r = new Runnable() {
			public void run() {
				Tick();
				handler.postDelayed(r, 100);
			}
		};
		handler.post(r);
	}

	private void checkConnectivity() {
		// Android allows Wifi connections while on Airplane mode, but no data
		// Check priority:
		// 		1 - Wifi
		// 		2 - Airplane mode
		// 		3 - Data
		if (WifiAvailable) {
			NetworkChanged(true);
		}
		// Android can be on Airplane mode AND connected to wifi
		else if (Settings.System.getInt(getApplicationContext().getContentResolver(), Settings.Global.AIRPLANE_MODE_ON, 0) != 0) {
			Log.d(Tag, "No active network, disabled");
			NetworkDisabled();
		} else {
			NetworkChanged(DataAvailable);
		}
	}

	/**
	 * Register to network connectivity changes
	 */
	void registerNetworkCallback() {
		NetworkRequest networkRequest = new NetworkRequest.Builder().build();
		networkCallback = new ConnectivityManager.NetworkCallback() {
			@Override
			public void onAvailable(@NonNull Network network) {
				super.onAvailable(network);
				updateNetworkConnection();
			}

			@Override
			public void onLost(@NonNull Network network) {
				super.onLost(network);
				updateNetworkConnection();
			}

		};
		connectivityManager.registerNetworkCallback(networkRequest, networkCallback);
	}

	void updateNetworkConnection() {
		WifiManager wifiManager = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
		if (wifiManager.isWifiEnabled()) {
			WifiInfo info = wifiManager.getConnectionInfo();
			WifiAvailable = info.getSupplicantState() == SupplicantState.COMPLETED;
		} else {
			WifiAvailable = false;
		}
		checkConnectivity();
	}

	/**
	 * Register to data connectivity changes
	 */
	void registerDataCallback() {
		phoneStateListener = new PhoneStateListener() {
			@Override
			public void onDataConnectionStateChanged(int state, int networkType) {
				super.onDataConnectionStateChanged(state, networkType);
				String message = "Current data network: ";
				switch (networkType) {
					case TelephonyManager.NETWORK_TYPE_EDGE:
					case TelephonyManager.NETWORK_TYPE_GPRS:
					case TelephonyManager.NETWORK_TYPE_CDMA:
						Log.d(Tag, message + " 2G");
						DataAvailable = false;
						break;

					case TelephonyManager.NETWORK_TYPE_UMTS:
					case TelephonyManager.NETWORK_TYPE_HSDPA:
					case TelephonyManager.NETWORK_TYPE_HSPA:
					case TelephonyManager.NETWORK_TYPE_HSPAP:
						Log.d(Tag, message + " 3G");
						DataAvailable = true;
						break;

					case TelephonyManager.NETWORK_TYPE_LTE:
						Log.d(Tag, message + " 4G");
						DataAvailable = true;
						break;

					case TelephonyManager.NETWORK_TYPE_NR:
						Log.d(Tag, message + " 5G");
						DataAvailable = true;
						break;

					default:
						Log.d(Tag, message + " unknown");
						DataAvailable = false;
						break;
				}

				checkConnectivity();
			}
		};
		telephonyManager.listen(phoneStateListener, PhoneStateListener.LISTEN_DATA_CONNECTION_STATE);
	}

	/**
	 * Unregister to network connectivity changes
	 */
	void unregisterNetworkCallback() {
		if (networkCallback != null) {
			connectivityManager.unregisterNetworkCallback(networkCallback);
			networkCallback = null;
		}
	}

	/**
	 * Unregister to data connectivity changes
	 */
	void unregisterDataCallback() {
		if (phoneStateListener != null) {
			telephonyManager.listen(phoneStateListener, PhoneStateListener.LISTEN_NONE);
			phoneStateListener = null;
		}
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		if (handler != null) {
			handler.removeCallbacks(r);
		}
		unregisterNetworkCallback();
		unregisterDataCallback();
	}

	@Override
	public void onPause() {
		super.onPause();
		Suspend();
	}

	@Override
	public void onResume() {
		super.onResume();
		Resume();
	}

	@Override
	public void onBackPressed() {
		moveTaskToBack(false);
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		/** Pass instance of main activity class to JNI for calling UI functions */
		PassMainActivityInstance();

		/** Pass Application activity to EOSDK Java side */
		EOSSDK.init(this);

		setContentView(R.layout.activity_main);
		console = findViewById(R.id.log);

		loginLayout = findViewById(R.id.loginLayout);
		loginSpinner = findViewById(R.id.loginSpinner);
		loginBtn = findViewById(R.id.loginBtn);
		loginBtn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				/** If the Login button is pressed, request a login to the Auth Interface using the mobile login flow */
				LoginInProgress();
				LoginWithAccountPortal();
			}
		});

		username = findViewById(R.id.username);
		logoutLayout = findViewById(R.id.logoutLayout);
		logoutSpinner = findViewById(R.id.logoutSpinner);
		logoutBtn = findViewById(R.id.logoutBtn);
		logoutBtn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				/** If the Logout button is pressed, start the logout flow for mobile devices
				 *  Any saved persistent credentials will be deleted */
				bShowLoginSpinner = true;
				Logout();
				UpdateUIState();
			}
		});

		LoginStateChanged(IsLoggedIn());

		/** Initialize SDK and start tick */
		boolean result = InitializeSDK(getFilesDir().getAbsolutePath() + "/");
		if (!result) {
			Log.e(Tag, "Failed to initialize SDK");
			return;
		}

		Log.d(Tag, "SDK initialized");

		/** Create platform allows us to to access the various interfaces and identify our application settings with 
		 values we obtained from the Dev Portal (see gradle.properties) */
		result = CreatePlatform(
			BuildConfig.ProductId,
			BuildConfig.SandboxId,
			BuildConfig.DeploymentId,
			BuildConfig.ClientId,
			BuildConfig.ClientSecret,
			false,
			0);

		if (!result) {
			Log.e(Tag, "Failed to create platform");
			return;
		}

		startGameLoop();

		Log.d(Tag, "Platform created");

		connectivityManager = getSystemService(ConnectivityManager.class);
		telephonyManager = getSystemService(TelephonyManager.class);

		/** Register connectivity changes */
		registerNetworkCallback();
		registerDataCallback();
		updateNetworkConnection();
		UpdateUIState();
	}

	/**
	 * Helper function to view text on screen
	 */
	public void ShowText(String text) {
		if (console != null) {
			text = text + "\n";
			console.append(text);
		}
	}

	/**
	 * Helper function to clear text on screen
	 */
	public void ClearText() {
		if (console != null) {
			console.setText("");
		}
	}

	/**
	 * Helper function to hide/show Login/Logout button
	 */
	public void LoginStateChanged(boolean loggedIn) {
		username.setText(GetUsername());
		bShowLoginSpinner = false;
		bLoggedIn = loggedIn;
		UpdateUIState();
	}

	public void LoginInProgress() {
		bShowLoginSpinner = true;
		UpdateUIState();
	}

	public void UpdateUIState() {
		loginLayout.setVisibility(bLoggedIn ? View.GONE : View.VISIBLE);
		loginSpinner.setVisibility(bShowLoginSpinner ? View.VISIBLE : View.GONE);
		loginBtn.setVisibility(bShowLoginSpinner ? View.GONE : View.VISIBLE);

		logoutLayout.setVisibility(bLoggedIn ? View.VISIBLE : View.GONE);
		logoutSpinner.setVisibility(bShowLoginSpinner ? View.VISIBLE : View.GONE);
		logoutBtn.setVisibility(bShowLoginSpinner ? View.GONE : View.VISIBLE);
	}

	public void OnDisplaySettingsUpdatedCallback(boolean bIsVisible) {

	}

	/**
	 * Native functions callable from java
	 */
	public native boolean InitializeSDK(String Path);

	public native boolean CreatePlatform(String ProductID, String SandboxID, String DeploymentId, String ClientId, String ClientSecret, boolean IsServer, int Flags);

	public native void Logout();

	public native void Tick();

	public native void Suspend();

	public native void Resume();

	public native void NetworkChanged(boolean connected);

	public native void NetworkDisabled();

	public native void LoginWithAccountPortal();

	public native boolean IsLoggedIn();

	public native String GetUsername();

	public native void PassMainActivityInstance();
}


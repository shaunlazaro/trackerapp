using System.Collections;
using System.Threading;
using UnityEngine;
using UnityEngine.Android;
using System;
using TechTweaking.Bluetooth;

public class BluetoothManager : MonoBehaviour
{
    public static BluetoothManager Instance;
    
    private void Awake()
    {
        if (Instance != null && Instance != this)
            Destroy(this);
        else
            Instance = this;
    }

    private BluetoothDevice arduino;
    
    public bool Connected { get => arduino != null && arduino.IsConnected; }

    void Start()
    {
        if(!Permission.HasUserAuthorizedPermission("android.permission.BLUETOOTH"))
            Permission.RequestUserPermission("android.permission.BLUETOOTH");
        BluetoothAdapter.OnDeviceOFF += HandleOnDeviceOff;//This would mean a failure in connection! the reason might be that your remote device is OFF

        BluetoothAdapter.OnDeviceNotFound += HandleOnDeviceNotFound; //Because connecting using the 'Name' property is just searching, the Plugin might not find it!(only for 'Name').

        arduino = new BluetoothDevice();
    }

    public void AttemptConnection()
    {
        Debug.Log("Attempt Connection");
        UIManager.Instance.UpdateConnectionStatus("Attempting to connect to bluetooth");
        if (BluetoothAdapter.isBluetoothEnabled())
        {

            UIManager.Instance.UpdateConnectionStatus("Bluetooth enabled. Trying to connect...");
            //arduino.MacAddress = "XX:XX:XX:XX:XX:XX";
            // Use one or the other
            arduino.Name = "solar_tracker_prototype";
            arduino.ReadingCoroutine = ReadingRoutine;
            arduino.connect();

        }
        else
        {
            UIManager.Instance.UpdateConnectionStatus("Bluetooth Disabled");
            BluetoothAdapter.OnBluetoothStateChanged += HandleOnBluetoothStateChanged;
            BluetoothAdapter.listenToBluetoothState(); // if you want to listen to the following two events  OnBluetoothOFF or OnBluetoothON
            BluetoothAdapter.askEnableBluetooth();//Ask user to enable Bluetooth
        }
    }

    public IEnumerator Send(string dataToSend)
    {
        if (arduino != null && !string.IsNullOrEmpty(dataToSend))
        {
            arduino.send(System.Text.Encoding.ASCII.GetBytes(dataToSend+ (char)10));//10 is our seperator Byte (sepration between packets)
        }
        yield break;
    }

    IEnumerator ReadingRoutine(BluetoothDevice device)
    {
        UIManager.Instance.UpdateNotification("Reading...");
        UIManager.Instance.UpdateConnectionStatus(UIManager.CONNECTED_STATUS_SUCCESS);
        while (device.IsReading)
        {
            byte[] msg = device.read();
            if (msg != null)
            {
                string content = System.Text.ASCIIEncoding.ASCII.GetString(msg);
                UIManager.Instance.UpdateNotification(content);
            }
            yield return null;
        }
    }

    public IEnumerator KillConnection()
    {
        arduino.close();
        UIManager.Instance.UpdateConnectionStatus("Disconnected");
        yield break;
    }

    void HandleOnBluetoothStateChanged(bool isBtEnabled)
    {
        if (isBtEnabled)
        {
            AttemptConnection();
            BluetoothAdapter.OnBluetoothStateChanged -= HandleOnBluetoothStateChanged;
            BluetoothAdapter.stopListenToBluetoothState();
        }
    }
    //This would mean a failure in connection! the reason might be that your remote device is OFF
    void HandleOnDeviceOff(BluetoothDevice dev)
    {
        UIManager.Instance.UpdateConnectionStatus("Device Off Error");
    }

    //Because connecting using the 'Name' property is just searching, the Plugin might not find it!.
    void HandleOnDeviceNotFound(BluetoothDevice dev)
    {
        UIManager.Instance.UpdateConnectionStatus("Device Not Found Error");
    }
}

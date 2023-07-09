using System.Collections;
using System.Threading;
using System.IO.Ports;
using UnityEngine;
using System;

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


    Thread listenThread;
    public SerialPort device = null;
    public bool continueRead = false;

    public void AttemptConnection(string port, int rate = 115200)
    {
        try
        {
            device = new SerialPort(port, rate);
            device.WriteTimeout = 500;
            device.ReadTimeout = 500;
            device.Open();

            continueRead = true;
            listenThread = new Thread(Read);
            listenThread.Start();


            UIManager.Instance.UpdateConnectionStatus(UIManager.CONNECTED_STATUS_SUCCESS);
        }
        catch (Exception e) 
        {
            KillConnection();
            Debug.LogException(e);
            UIManager.Instance.UpdateConnectionStatus(e.Message);
        }
    }

    public IEnumerator KillConnection()
    {
        if (!continueRead) yield break; // Jank way to prevent race condition type of error.

        UIManager.Instance.UpdateConnectionStatus("Disconnecting...");
        continueRead = false;
        if (listenThread != null && listenThread.IsAlive)
        {
            listenThread.Join(5000);
        }
        device.Close();
        UIManager.Instance.UpdateConnectionStatus("Disconnected");
    }

    public static void Read()
    {
        while (Instance.continueRead)
        {
            try
            {
                string message = Instance.device.ReadLine();
                Console.WriteLine(message);
            }
            catch (TimeoutException) { }
        }
        Debug.Log("Read Thread Done");
    }
}

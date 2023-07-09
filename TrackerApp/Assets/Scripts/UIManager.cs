using System.Collections;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class UIManager : MonoBehaviour
{
    public static UIManager Instance { get; private set; }

    [SerializeField]
    TextMeshProUGUI portsList;
    [SerializeField]
    TMP_Dropdown portsSelector;

    [SerializeField]
    TextMeshProUGUI status;

    public const string CONNECTED_STATUS_SUCCESS = "Connected";
    string statusConnection = "Not Connected";
    string statusLocation = "Not Connected";
    
    [SerializeField]
    [Multiline]
    string instructions;
    [SerializeField]
    [Multiline]
    string noConnectionInstructions;

    [SerializeField]
    Button bluetoothButton;
    [SerializeField]
    Button bluetoothDisconnectButton;
    [SerializeField]
    Button gpsButton;

    private void Awake()
    {
        if (Instance != null && Instance != this)
            Destroy(this);
        else
            Instance = this;
    }

    // Start is called before the first frame update
    void Start()
    {
        RefreshBluetoothPortsDisplayed();
        UpdateStatus();
        LockBluetoothUI(true);
    }

    public void OnClickRefreshPorts()
    {
        RefreshBluetoothPortsDisplayed();
    }
    public void OnClickBluetoothConnect()
    {
        LockBluetoothUI(false);
        BluetoothManager.Instance.AttemptConnection(portsSelector.options[portsSelector.value].text);
    }
    public void OnClickBluetoothDisconnect()
    {
        StartCoroutine(BluetoothManager.Instance.KillConnection());
    }
    public void OnClickInitLocation()
    {
        if (!Location.Instance.initialized && !Location.Instance.initializing) 
            StartCoroutine(Location.Instance.InitLocation());
    }

    public void RefreshBluetoothPortsDisplayed()
    {
        string[] ports = SerialPort.GetPortNames();
        string portDisplayText = "List Of Available Ports: ";
        foreach (string port in ports) { portDisplayText += $"\n{port}"; }
        portsList.text = portDisplayText;
        portsSelector.ClearOptions();
        List<string> optionsList = ports.ToList<string>();
        optionsList.Insert(0, "Bluetooth Port:");
        portsSelector.AddOptions(optionsList);
        portsSelector.value = 0;
        Debug.Log(portsSelector.options[0].text);
    }

    public void UpdateConnectionStatus(string statusText)
    {
        statusConnection = statusText;
        if (statusText == CONNECTED_STATUS_SUCCESS)
            LockBluetoothUI(false);
        else
            LockBluetoothUI(true);

        UpdateStatus();
    }
    public void UpdateLocationStatus(string statusText)
    {
        statusLocation = statusText;
        UpdateStatus();
    }

    public void LockBluetoothUI(bool interactable)
    {
        portsSelector.interactable = interactable;
        bluetoothButton.interactable = interactable;
        bluetoothDisconnectButton.interactable = !interactable;
    }

    public void UpdateStatus()
    {
        status.text = $"Connection Status: {statusConnection}\nLocation Status: {statusLocation}\n\nInstructions:\n";
        status.text += statusConnection == CONNECTED_STATUS_SUCCESS && statusLocation == CONNECTED_STATUS_SUCCESS ? instructions : noConnectionInstructions;
    }
}

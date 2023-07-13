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
    TextMeshProUGUI status;
    [SerializeField]
    TextMeshProUGUI notificationText;

    public const string CONNECTED_STATUS_SUCCESS = "Connected";
    string statusConnection = "Not Connected";
    string statusLocation = "Not Connected";
    string notificationString = "";

    [SerializeField]
    [Multiline]
    string noConnectionInstructions;

    [SerializeField]
    Button bluetoothButton;
    [SerializeField]
    Button bluetoothDisconnectButton;
    [SerializeField]
    Button gpsButton;
    [SerializeField]
    Button configureTrackerButton;
    [SerializeField]
    Button moveMotorLeftButton;
    [SerializeField]
    Button moveMotorRightButton;
    [SerializeField]
    Button setMotorPosition0Button;

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
        RefreshGUI();
        LockBluetoothUI(true);
    }

    public void OnClickBluetoothConnect()
    {
        LockBluetoothUI(false);
        BluetoothManager.Instance.AttemptConnection();
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
    public void OnClickSendLocation()
    {
        if(Location.Instance.initialized && BluetoothManager.Instance.Connected)
            StartCoroutine(BluetoothManager.Instance.Send(Location.Instance.latitude.ToString()));
    }
    public void OnClickMoveMotorLeft()
    {
        StartCoroutine(BluetoothManager.Instance.Send("-25"));
    }
    public void OnClickMoveMotorRight()
    {
        StartCoroutine(BluetoothManager.Instance.Send("25"));
    }
    public void OnClickZeroMotorPosition()
    {
        StartCoroutine(BluetoothManager.Instance.Send("SET"));
    }

    public void UpdateConnectionStatus(string statusText)
    {
        statusConnection = statusText;
        if (statusText == CONNECTED_STATUS_SUCCESS)
            LockBluetoothUI(false);
        else
            LockBluetoothUI(true);

        RefreshGUI();
    }
    public void UpdateLocationStatus(string statusText)
    {
        statusLocation = statusText;
        RefreshGUI();
    }
    public void UpdateNotification(string statusText)
    {
        notificationString = statusText;
        RefreshGUI();
    }

    public void LockBluetoothUI(bool interactable)
    {
        bluetoothButton.interactable = interactable;
    }

    public void RefreshGUI()
    {
        bool btConnected = statusConnection == CONNECTED_STATUS_SUCCESS;
        bool locationConnected = statusLocation == CONNECTED_STATUS_SUCCESS;

        string instructions = $"Please set the tilt angle to: {(locationConnected ? Location.Instance.latitude.ToString() : "NAN")}.\n" +
            $"After clicking \"Upload Correct Angle,\" the red LED will turn off when the angle is acceptable.\n" +
            $"Once the angle is acceptable, use the buttons to rotate the panel until it is flat, then click \"Calibrate Position\"";

        status.text = $"Connection Status: {statusConnection}\nLocation Status: {statusLocation}\n\nInstructions:\n";
        status.text += btConnected && locationConnected ? instructions : noConnectionInstructions;
        notificationText.text = notificationString;

        bluetoothDisconnectButton.interactable = btConnected;
        gpsButton.interactable = !locationConnected;
        // bluetoothButton.interactable = !btConnected; -> We don't do this because we lock while attempting to connect.
        configureTrackerButton.interactable = btConnected && locationConnected;
        moveMotorLeftButton.interactable = btConnected && locationConnected;
        moveMotorRightButton.interactable = btConnected && locationConnected;
        setMotorPosition0Button.interactable = btConnected && locationConnected;
    }
}

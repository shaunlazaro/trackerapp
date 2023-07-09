using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Location : MonoBehaviour
{
    public static Location Instance { get; private set; }

    public bool initialized = false;
    public bool initializing = false;
    public float latitude;
    public float longitude;

    private void Awake()
    {
        if (Instance != null && Instance != this)
            Destroy(this);
        else
            Instance = this;
    }

    private void Start()
    {
        StartCoroutine(InitLocation());
    }

    // Mostly copied from Unity Documentation
    public IEnumerator InitLocation()
    {
        if (initializing) yield break;

        // Reset:
        initializing = true;
        initialized = false;
        latitude = 0;
        longitude = 0;

        UIManager.Instance.UpdateLocationStatus("Attempting to connect to location services...");

        // Check if the user has location service enabled.
        if (!Input.location.isEnabledByUser)
        {
            UIManager.Instance.UpdateLocationStatus("Location access denied");
            initializing = false;
            yield break;
        }


        UIManager.Instance.UpdateLocationStatus("Attempting to initialize location services...");

        // Starts the location service.
        Input.location.Start();

        // Waits until the location service initializes
        int maxWait = 20;
        while (Input.location.status == LocationServiceStatus.Initializing && maxWait > 0)
        {
            yield return new WaitForSeconds(1);
            maxWait--;
        }

        // If the service didn't initialize in 20 seconds this cancels location service use.
        if (maxWait < 1)
        {
            Debug.Log("Timed out"); 
            UIManager.Instance.UpdateLocationStatus("Timed out");
            initializing = false;
            yield break;
        }

        // If the connection failed this cancels location service use.
        if (Input.location.status == LocationServiceStatus.Failed)
        {
            UIManager.Instance.UpdateLocationStatus("Unable to determine device location");
            Debug.Log("Unable to determine device location");
            initializing = false;
            yield break;
        }
        // Connection success.
        else
        {
            UIManager.Instance.UpdateLocationStatus("Detecting Location...");
            for (int i = 0; i < 100; i++)
            {
                yield return new WaitForSeconds(1);
                if (Input.location.status == LocationServiceStatus.Running)
                    break;
            }

            initialized = true;
            latitude = Input.location.lastData.latitude;
            longitude = Input.location.lastData.longitude;
            UIManager.Instance.UpdateLocationStatus(UIManager.CONNECTED_STATUS_SUCCESS);
            Debug.Log("Location: " + Input.location.lastData.latitude + " " + Input.location.lastData.longitude + " " + Input.location.lastData.altitude + " " + Input.location.lastData.horizontalAccuracy + " " + Input.location.lastData.timestamp);
        }

        initializing = false;
        // Stops the location service if there is no need to query location updates continuously.
        Input.location.Stop();
    }
}

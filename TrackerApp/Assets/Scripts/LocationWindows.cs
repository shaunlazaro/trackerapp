/*using Windows.Devices;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LocationWindows : MonoBehaviour
{
    public static LocationWindows Instance { get; private set; }

    public bool initialized = false;
    public bool initializing = false;
    public float latitude;
    public float longitude;

    GeoCoordinateWatcher watcher;

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

    public IEnumerator InitLocation()
    {
        initializing = true;
        initialized = false;

        GeoCoordinate location = new GeoCoordinate();

        yield break;
    }
}
*/
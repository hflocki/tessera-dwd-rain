# Rain (DWD)

When it rains in the next two hours, from a DWD precipitation forecast sensor in Home Assistant: the icon in a circle
that turns blue while it rains or will within the hour, a sentence such as "In 25 min. it rains for 20 min." or "No
rain in the next two hours", the rain now in mm/h, and under it the forecast of the next two hours as bars (light rain
blue, a downpour purple).

## Set up

1. Have a DWD precipitation forecast sensor in Home Assistant whose attributes are `next_start`, `next_end`,
   `next_length` (minutes) and `forecast` (a time and a rain rate in mm/h for every five minutes), such as
   `sensor.dwd_precipitation_forecast_home`. Its state is the rain now.
2. In Tessera, open **Plugins**, **Add with a link**, and paste the link of this repository. Add the plugin to a
   screen; the screen builds.
3. In **Layout**, place the **Rain** tile (2 x 1 or wider) and choose the sensor in its inspector. Only sensors with a
   `forecast` are offered.

## Options

- **Show the chart**: the bars under the words, where the tile has room for them.
- **Forecast step**: the minutes between two values of the forecast, 5 for the DWD nowcast.

## Good to know

- The tile counts down on the screen's own clock; the sensor brings a new forecast every five minutes.
- The chart starts at the first value of the forecast, which the sensor keeps at now.
- It uses no service of its own: everything comes from the sensor in Home Assistant.

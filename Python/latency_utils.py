#!/usr/bin/env python3
# Copyright 2021, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

import numpy as np
import pandas as pd
from scipy import interpolate, signal

RESAMPLE_RESOLUTION_MICROSECONDS = 10


def _rescale(x):
    half_range = (x.max() - x.min())/2
    middle = x.min() + half_range
    return (x - middle)/half_range


def ms_to_periods(ms): return int(ms * 1000 / RESAMPLE_RESOLUTION_MICROSECONDS)


def format_datetime(dt):
    return f"{dt.second * 1000 + (dt.microsecond // 1000)}ms"


def plot_overlaid(df, ticks_ms=20, *args, **kwargs):
    ticks = np.arange(df.index.min(), df.index.max(),
                      pd.to_timedelta(ticks_ms, unit='ms'))
    # labels = pd.Series(ticks).apply(format_datetime)
    ax = df[['brightness', 'rot']].plot(xticks=ticks, *args, **kwargs)
    my_df = df.assign(pos=((df['rot'] - df['rot'].shift(1)) > 0))
    for dirchange in my_df[my_df['pos'] != my_df['pos'].shift(1)].index:
        ax.axvline(dirchange, color='green', label=str(dirchange), linewidth=1)
    return ax


def interpolate_with_spline(df):
    xnew = np.arange(df['us'].min(), df['us'].max(),
                     RESAMPLE_RESOLUTION_MICROSECONDS)
    x = df['us'].to_numpy()
    newDf = pd.DataFrame({'us': pd.Series(xnew).astype(int)})
    for colName in df.columns:
        if colName in ('us', 'timestamp'):
            continue
        s = interpolate.InterpolatedUnivariateSpline(x, df[colName].to_numpy())
        ynew = s(xnew)
        newDf[colName] = ynew

    # Make this a datetime indexed frame, by converting our timestamps
    newDf = (newDf
             .assign(timestamp=pd.to_datetime(newDf['us'], unit='us'))
             .set_index('timestamp'))
    return newDf


def filter_brightness(df):
    fs = 1000000/RESAMPLE_RESOLUTION_MICROSECONDS
    buttersos = signal.butter(1, 60, output='sos', fs=fs)
    filtered = signal.sosfilt(buttersos, df['brightness'].to_numpy())
    return df.assign(brightness=filtered)


def cleanup(df, rot_axis='z'):
    # Make this a datetime indexed frame, by converting our timestamps
    # df = (df
    #       .assign(timestamp=pd.to_datetime(df['us'], unit='us'))
    #       .set_index('timestamp'))
    # # Resample and interpolate to a steady period
    # df = (df
    #       .resample(RESAMPLE_RESOLUTION_MICROSECONDS *
    #                 pd.tseries.offsets.Micro())
    #       .interpolate(method='time'))
    df = interpolate_with_spline(df)
    # Compute incremental rotation from the rate
    incrot = df[['dr' + rot_axis]] * RESAMPLE_RESOLUTION_MICROSECONDS/1000000.0
    df = df.assign(rot=incrot.cumsum())

    # df = df.assign(brightness=_rescale(df['brightness']))
    return df


def rescale_brightness(df):
    return df.assign(brightness=_rescale(df['brightness']))


def read_with_headers(fn, *args, **kwargs):
    df = pd.read_csv(fn)
    return cleanup(df, *args, **kwargs)


def read_without_headers(fn, *args, **kwargs):
    df = pd.read_csv(fn, names=['us', 'drx', 'dry', 'drz', 'brightness'])
    df = df.dropna(axis=0)
    return cleanup(df, *args, **kwargs)


def second_to_timestamp_micros(sec): return pd.Timestamp(
    pd.to_datetime(sec, unit='s'), freq='us')


def extract_range(df, start_sec, end_sec):
    subsetted = (df[second_to_timestamp_micros(start_sec):second_to_timestamp_micros(end_sec)])[['brightness', 'rot']]
    # Now need to adjust the IMU zero point
    return subsetted.assign(rot=(subsetted['rot'] - subsetted['rot'].mean()))


def get_shifted_tracker(df, periods): return df['rot'].shift(periods=periods)


def get_frame_with_shifted_tracker(df, periods):
    return df.assign(
        rot=get_shifted_tracker(df, periods=periods))


def _compute_corr_with_lag(df: pd.DataFrame, pds: int):
    return get_shifted_tracker(df, pds).corr(df['brightness'])


def compute_cross_correlation(df, min_shift_ms, max_shift_ms, step_micros=1000):
    micros_list = list(
        range(min_shift_ms * 1000, max_shift_ms * 1000, step_micros))
    # some truncation may occur here if step_micros isn't a multiple of RESAMPLE_RESOLUTION_MICROSECONDS
    periods_list = [int(x / RESAMPLE_RESOLUTION_MICROSECONDS)
                    for x in micros_list]
    # so we calculate the index based on the periods, rather than the original microseconds.
    ms_list = [(x * RESAMPLE_RESOLUTION_MICROSECONDS) /
               1000.0 for x in periods_list]
    if step_micros % 1000 == 0:
        # only integer milliseconds
        ms_list = [int(x) for x in ms_list]
    cross_input = pd.DataFrame({'delay_periods': periods_list}, index=ms_list)
    result = cross_input.assign(
        correlation=cross_input['delay_periods'].apply(
            lambda x: _compute_corr_with_lag(df, x))
    )
    return result[['correlation']]


def get_max_corr_latency(crosscorr):
    return crosscorr['correlation'].idxmax()


if __name__ == '__main__':
    df = read_with_headers('latency2-trimmed.csv')
    df = extract_range(df, 28, 38)
    print(df)
    corr = compute_cross_correlation(df, 0, 100)
    latency = get_max_corr_latency(corr)

    print("Latency is near", latency)
    # now drill down
    corr = compute_cross_correlation(df, latency - 1, latency + 1, 10)

    latency = get_max_corr_latency(corr)
    print("Correlation is maximized for a latency of", latency, "ms")

#!/usr/bin/env python
#
# Copyright 2016-2022 Matt Evans
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# A hacky quick test of making Plotly graphs from live MR-ISS event counters

import datetime
import math
import socket
import time
from threading import Timer

import dash
import dash_core_components as dcc
import dash_html_components as html
import plotly.tools as pt
import plotly.graph_objs as go
from dash.dependencies import Input, Output

################################################################################

hostname = 'localhost'
port = 8888

################################################################################
# Internal state

counter = 0
speed = 0

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

time_start = time.time()

################################################################################
# Statistics

stat_history = []
stats_last_time = 0
stats_last_instrs = 0 # All gross, make this better.

# Want to achieve a number of things:
# - Give access to raw stats if required
# - But, summarise; e.g. classify instructions as:
#       Arithmetic, Logical, Memory, Branch, Compare, CondReg, Special, Trap
# - Also summarise types of exception
#
# This leads us to creating synthetic stat values.  Also, instructions vs time,
# TLB hit rate
#

def print_time():
    Timer(1, print_time).start()
    print "Fetching stats at ", time.time()
    get_stats()

def     gen_synthetic_stats(s):
    i = s.get('INST_DELTA')
    t = s.get('TIME_DELTA')
    if i != None and t != None:
        s['SYNTH_MIPS'] = int(i) / t
    # TODO: TLB hit rate
    # TODO: Automatic classification of instrs


def     get_stats():
    global stat_history
    global time_start
    global stats_last_time # static
    global stats_last_instrs # static

    client.send('STATS\n')
    response = client.recv(4096)
#    print "Stats: '%s'\n" % (response)

    stat_list = response.split(', ')
    stat_list.pop() # Last one's a newline
    stats = dict(s.split(' ') for s in stat_list)
    tnow = time.time() - time_start
    stats['TIME'] = tnow
    stats['TIME_DELTA'] = tnow - stats_last_time
    stats_last_time = tnow
    insts = stats.get('CTR_INST_EXECUTED')
    if insts == None:
        insts = 0
    else:
        insts = int(insts)
    stats['INST_DELTA'] = insts - stats_last_instrs
    stats_last_instrs = insts

    gen_synthetic_stats(stats)
    stat_history.append(stats)


################################################################################
# Dash stuff:  layout, callbacks

app = dash.Dash(__name__)
app.layout = html.Div(
    html.Div([
        html.H4('Interactive thing'),
        html.Div(id='live-update-text'),
        dcc.Graph(id='live-update-graph'),
        dcc.Interval(
            id='interval-component',
            interval=1*1000, # in milliseconds
            n_intervals=0
        ),
        html.H5('Data points'),
        dcc.Slider(
            id='width-slider',
            min=5,
            max=200,
            marks={i: str(i) for i in [5, 20, 50, 100, 150, 200]},
            value=20
        ),
        html.H5('Config'),
        dcc.Checklist(
            id='checkbox-component',
            options=[
                {'label': 'Relative', 'value': 'REL'},
            ],
            values=['REL']
        ),
        html.Div(id='status'),
    ])
)

@app.callback(Output('live-update-text', 'children'),
              [Input('interval-component', 'n_intervals')])
def update_metrics(n):
    global counter
    counter = counter + 1
    style = {'padding': '5px', 'fontSize': '16px'}
    return [
        html.Span('Time %s, count %d, interval %d' % (datetime.datetime.now(), counter, n), style=style)
    ]


# Multiple components can update everytime interval gets fired.
@app.callback(Output('live-update-graph', 'figure'),
              [Input('interval-component', 'n_intervals'),
               Input('width-slider', 'value'),
               Input('checkbox-component', 'values')
              ])
def update_graph_live(n, dpoints, checkboxes):
    # It isn't sufficient to check stats here -- the jitter on this callback is immense,
    # and particularly when we're looking for per-time-unit measurements, it all goes awry...
#    get_stats()

    # Collect some data (width from slider!)
    hist = stat_history[-dpoints:]

    stats_latest = stat_history[len(hist)-1]

    data = { i: [] for i in list(stats_latest) }


    if 'REL' in checkboxes:
        # Relative values
        last = hist[0]
        # The rest:
        if len(hist) > 1:
            h = hist[1:]
        else:
            h = hist
        for s in h:
            for i in list(s):
                if i != 'TIME' and i != 'SYNTH_MIPS':
                    data[i].append(int(s[i]) - int(last[i]))
                else:
                    data[i].append(s[i])
            last = s
    else:
        # Absolute values
        for s in hist:
            for i in list(s):
                data[i].append(s[i])

    # Create a line per counter:
    plot_data = []
    for i in list(stats_latest):
        plot_data.append({
                'x': data['TIME'],
                'y': data[i],
                'name': i,
                'mode': 'lines',
                'type': 'scatter'
            })

    plot_data_test = [
        # First line
        {
            'x': data['TIME'],
            'y': data['CTR_INST_EXECUTED'],
            'name': 'Inst',
            'mode': 'lines',
            'type': 'scatter'
        },
        # Second line
        {
            'x': data['TIME'],
            'y': data['SYNTH_MIPS'],
            'name': 'MIPS',
            'mode': 'lines',
            'type': 'scatter'
        }
        ]

    fig = go.Figure(data = plot_data,
                    layout = go.Layout(
                        width=1500,
                        height=800,
                        autosize=True
                    )
                )
    return fig



@app.callback(Output('status', 'children'),
              [Input('width-slider', 'value')])
def inputs_changed(checkbox_val, slider_val):
    return [
        html.Span('Action:  datapoints=%d' % (slider_val))
    ]


if __name__ == '__main__':
    # FIXME: Retry this till connect succeeds?
    client.connect((hostname, port))
    # Test success...?
    Timer(1, print_time).start()

    app.run_server(debug=False, host='0.0.0.0')

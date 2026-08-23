#
# MIT License
#
# Copyright (c) 2026 Czech Technical University in Prague
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.#
from typing import Dict

import pandas as pd


def generate_result_table(data: Dict[int, pd.DataFrame]):
    # add the length column
    dfs = []
    for length, df in data.items():
        dfs.append(df.assign(instance_length=length))

    # concat all dataframes
    out = pd.concat(dfs)

    # select output columns
    # out = pd.DataFrame(out[['method', 'cost_minutes', 'plan_count', 'comp_time_s', 'travel_time_to_start_per_plan', 'wait_time_per_plan', 'used_connections', 'instance_length']])
    out = pd.DataFrame(out.query('duration_minutes in (5, 15, 30)')[[
        'method',
        'batchl',
        'gglimit',
        'cost_minutes',
        'plan_count',
        'total_time',
        # 'used_connections',
        'duration_minutes'
    ]])

    # rename columns
    col_names = {
        'duration_minutes': "Dur. [min]",
        "method": "Method",
        'batchl': 'Batch [s]',
        'gglimit': 'Lim.',
        'total_time': 'Comp. time [s]',
        'cost_minutes': 'Total Cost [min]',
        'plan_count': 'Used Vehicles',
        'travel_time_to_start_per_plan': 'Avg. time to start [s]',
        'wait_time_per_plan': 'Avg. wait time per plan [s]',
        # 'used_connections': 'Connections'
    }
    out.rename(col_names, inplace=True, axis=1)

    # transform to multiindex table
    columns = [column for column in out.columns if column not in ['Method', 'Inst. horizon']]
    out['Dur. [min]'] = out['Dur. [min]'].astype(int)
    out['Batch [s]'] = out['Batch [s]'].astype(pd.Int32Dtype())
    out['Batch [s]'].fillna(0, inplace=True)
    out['Lim.'].fillna(0, inplace=True)
    out['Lim.'] = out['Lim.'].astype(bool)

    # rotate text for proposed method
    out['Method'] = [r'\rotatebox{90}{\parbox{1.7cm}{Plan Chaining with VGA}}' if method.startswith('prop') else method for method in out['Method']]

    pout = out.pivot_table(index=['Method', 'Batch [s]', 'Lim.'], columns='Dur. [min]', values=columns)

    # reorder columns to the original order
    pout = pout.reindex(columns, axis=1, level=0)
    pout.sort_values(by=['Method', 'Batch [s]', 'Lim.'], inplace=True)
    # pout.sort_values(by='Method', inplace=True, key=lambda col: pd.Series((get_method_value(value) for value in col)))

    # transform int columns back to int
    lengths = [5, 15, 30]
    columns_to_transform_back_to_int = ['Total Cost [min]', 'Used Vehicles']
    for c in columns_to_transform_back_to_int:
        for l in lengths:
            # pout.loc[:, (c, l)] = pout.loc[:, (c, l)].astype(pd.Int32Dtype())
            if pout.loc[:, (c, l)].hasnans:
                pout.loc[:, (c, l)] = pout.loc[:, (c, l)].astype(pd.Int32Dtype())
            else:
                pout[(c, l)] = pout[(c, l)].astype(int)

    # adjust style and print the table
    styler = pout.style \
        .hide(axis=1, level=1) \
        .format_index(escape='latex', axis=1, level=[0, 1]) \
        .format_index(escape='latex', axis=0, level=[1]) \
        .format_index(axis=0, level=2, formatter=lambda x: 'yes' if x else 'no') \
        .format_index(axis=0, level=1, formatter=lambda x: '-' if x == 0 else x) \
        .format(escape='latex', precision=2, na_rep='-') \
        .highlight_min(
        subset=["Total Cost [min]", 'Used Vehicles', 'Comp. time [s]'],
        color=None,
        props='font-weight: bold'
    )

    result_string = styler.to_latex(
        convert_css=True,
        multirow_align='c',
        multicol_align='c',
        hrules=True, # for main hlines
        clines="skip-last;data" # for hlines between multirows
    )

    # result string manipulation
    durations = list(pout.columns.get_level_values(1))
    replacement = ' & '.join([f"\\SI{{{dur}}}{{min}}" for dur in durations])
    result_string = result_string.replace('&  &  &  &  &  &  &  &  &', f"& {replacement}")

    print(result_string)

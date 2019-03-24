import sys
import numpy as np
import matplotlib.pyplot as plt
import math

if __name__ == '__main__':
    if __name__ == '__main__':
        objects = ('local-single addition',
                   'local-empty func',
                   'local-trap',
                   'VM-single addition',
                   'VM-empty func',
                   'VMM-trap',
                   'container-single addition',
                   'container-empty func',
                   'container-trap')
        n_groups = 3
        # create data of the plot
        # means_local = tuple([math.log(10, float(x)) for x in sys.argv[1:4]])
        # means_vm = tuple([math.log(10, float(x)) for x in sys.argv[4:7]])
        # means_container = tuple([math.log(10, float(x)) for x in sys.argv[7:]])
        means_local = tuple([float(x) for x in sys.argv[1:3]] + [math.log(float(sys.argv[4]))])
        means_vm = tuple([float(x) for x in sys.argv[4:6]] + [math.log(float(sys.argv[6]))])
        means_container = tuple([float(x) for x in sys.argv[7:9]] + [math.log(float(sys.argv[9]))])

        # create plot
        fig, ax = plt.subplots()
        index = np.arange(n_groups)
        bar_width = 0.1
        opacity = 0.8
        rects1 = plt.bar(index, means_local, bar_width, alpha=opacity, color='r', label='Local Machine')
        rects2 = plt.bar(index + bar_width, means_vm, bar_width, alpha=opacity, color='g', label='Virtual Machine')
        rects3 = plt.bar(index + bar_width * 2, means_container, bar_width, alpha=opacity, color='b', label='Container')
        rects_arr = [rects1, rects2, rects3]
        for rects in rects_arr:
            c = 0
            for rect in rects:
                c += 1
                height = rect.get_height()
                ax.text(rect.get_x() + rect.get_width() / 2., 1.05 * height,
                        str(float(height))[:5],
                        ha='center', va='bottom')
        plt.xlabel('Function type')
        plt.ylabel('Time to run')
        plt.title('EX1-Comparison')
        plt.xticks(index + bar_width, ('Single addition', 'Empty Function', 'Trap(in log10)'))
        plt.legend()

        plt.tight_layout()
        # plt.show()
        plt.savefig('graph.png')

    # import plotly
    #
    # plotly.tools.set_credentials_file(username='snirsh', api_key=api)
    # data = [go.Bar(
    #     x=[
    #
    #     ],
    #     y=[int(x) for x in sys.argv[1::]]
    # )]
    # py.iplot(data, filename='results')

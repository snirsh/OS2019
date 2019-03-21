import sys
import numpy as np
import matplotlib.pyplot as plt


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
        means_local = tuple([int(x) for x in sys.argv[1:4]])
        means_vm = tuple([int(x) for x in sys.argv[4:7]])
        means_container = tuple([int(x) for x in sys.argv[7:]])

        # create plot
        fig, ax = plt.subplots()
        index = np.arange(n_groups)
        bar_width = 0.1
        opacity = 0.8
        rects1 = plt.bar(index, means_local, bar_width, alpha=opacity, color='r', label='Local Machine')
        rects2 = plt.bar(index + bar_width, means_vm, bar_width, alpha=opacity, color='g', label='Virtual Machine')
        rects3 = plt.bar(index + bar_width*2, means_container, bar_width, alpha=opacity, color='b', label='Container')
        plt.xlabel('Function type')
        plt.ylabel('Time to run')
        plt.title('EX1-Comparison')
        plt.xticks(index + bar_width, ('Single addition', 'Empty Function', 'Trap'))
        plt.legend()

        plt.tight_layout()
        # plt.show()
        plt.savefig('graph.png')

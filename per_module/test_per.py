import per
import numpy as np

def main():
    p = per.create(capacity=256, alpha=0.6, beta=0.4)
    # # add python objects
    # per.add(p, [1, 2, 3], 0.5)                    # list
    # per.add(p, {"s": 1, "a": 2}, 0.4)
    # per.add(p, np.zeros((4, 5), dtype=np.float32), 0.6)
    # per.add(p, np.ones((2, 3), dtype=np.float32), 0.6)

    # # sample
    # res = per.sample(p, 5)
    # print(res)

    # later update priorities using td-errors
    # td_errors = [0.7, -1.3]
    # per.update_priorities(p, p_indices, td_errors)
    pass


if __name__ == "__main__":
    main()
import random
import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt
import numpy
import intervals as intervals

def randX():
    return random.random()

def randY(x):
    if((0<=x and x<=0.25) or (0.5<=x and x<=0.75)):
        return numpy.random.binomial(1, 0.8)
    return numpy.random.binomial(1, 0.1)

def sample_points(m = 100) :
    points_x = [randX() for i in range(m)]
    pnts = sorted([(x,randY(x)) for x in points_x], key=lambda x:x[0])
    new_x=[pair[0] for pair in pnts]
    new_y=[pair[1] for pair in pnts]
    return new_x, new_y


def plot_points_A():

    plt.plot([0.25,0.25],[0,1],color="black")
    plt.plot([0.5,0.5],[0,1],color="black")
    plt.plot([0.75,0.75],[0,1],color="black")
    #print (pairs)
    new_x, new_y = sample_points()
    plt.scatter(new_x, new_y)
    #print (new_x)
    best_res,be=intervals.find_best_interval(new_x,new_y,2)
    for tup in best_res:
        plt.axvspan(tup[0],tup[1],alpha=0.5,color="yellow")
    plt.xlabel("x")
    plt.ylabel("y")
    plt.savefig('a.png')
def testsC():
    emp_res = []
    true_res = []
    T= 100
    for m in range(10, 105, 5):
        emp_errs = []
        true_errs = []
        for t in range(0,T):
            x,y = sample_points(m)
            erm_intervals, erm_count_err = intervals.find_best_interval(x,y,2)
            emp_errs.append(erm_count_err/m)
            true_errs.append(calcTrueError(erm_intervals))
        emp_res.append((m, sum(emp_errs)/T))
        true_res.append((m, sum(true_errs)/T))
    plt.plot([x[0] for x in emp_res], [x[1] for x in emp_res], color = "yellow", label = "Emp error")
    plt.plot([x[0] for x in emp_res], [x[1] for x in true_res], color = "purple", label = "True error")
    plt.xlabel("m")
    plt.legend()
    plt.savefig("c.png")


def testD():
    hypo=[0 for x in range(1,21)]
    x,y = sample_points(50)
    emp_err_res = []
    true_err_res = []
    for k in range(1,21):
        erm_intervals, erm_count_err = intervals.find_best_interval(x,y,k)
        emp_err_res.append(erm_count_err/50)
        true_err_res.append(calcTrueError(erm_intervals))
        hypo[k-1] = erm_intervals
    plt.plot([x for x in range(1,21)], [x for x in emp_err_res], color = "yellow", label = "Emp Error")
    plt.plot([x for x in range(1,21)], [x for x in true_err_res], color = "purple", label = "True Error")
    plt.legend()
    plt.annotate('k*', xy=(emp_err_res.index(min(emp_err_res))+1, min(emp_err_res)))
    plt.xlabel("k")
    plt.savefig("d.png")
    print("k* is {}".format(hypo[emp_err_res.index(min(emp_err_res))]))
    return hypo

def calcOverlap(intv_1,intv_2): #returns the length of intersection between
    return max(0,min(intv_1[1],intv_2[1])- max(intv_1[0],intv_2[0]))

def calcTrueError(best_res): #best_res is the interval returned from find_best_interval
    zero_intv_lst=[]
    if (best_res[0][0]!=0): #add left edge if needed
        zero_intv_lst.append((0,best_res[0][0]))

    for i in range(0,len(best_res)-1): #zero interval is the interval between
        zero_intv_lst.append((best_res[i][1],best_res[i+1][0]))

    if (best_res[-1][1]!=1): #add right edge if needed
        zero_intv_lst.append((best_res[-1][1],1))

    total_error=0
    for zero_intv in zero_intv_lst:
        total_error+=calcError(zero_intv,0)
    for one_intv in best_res:
        total_error+=calcError(one_intv,1)

    return total_error

def calcError(interval,typ): #(interval,type) where type is 0 or 1
    interval_length=interval[1]-interval[0]
    overlap_08=calcOverlap(interval,(0,0.25))+calcOverlap(interval,(0.5,0.75))
    overlap_01=calcOverlap(interval,(0.25,0.5))+calcOverlap(interval,(0.75,1))
    if (typ==1):
        return ((overlap_08*0.2+overlap_01*0.9)) #we got 1, Pr for 1 to get 0
    return overlap_08*0.8+overlap_01*0.1 #we got 0, Pr for y to get 1

def crossValid(hyps):
    x,y = sample_points(50)
    emp_err = [0 for num in range(1,21)] #for each k
    for i in range (0,len(hyps)):
        counter = 0
        for j in range(0,50):
            if (checkHypOnX(x[j], hyps[i]) != y[j]):
                counter+=1
        emp_err[i] = counter/50
    return hyps[emp_err.index(min(emp_err))]

def checkHypOnX(x, hyp):
    for interval in hyp:
        if (interval[0] <=x and interval[1] >= x):
            return 1
    return 0
plot_points_A()

hyps = testD()
hyp = crossValid(hyps)

testsC()
print("\nIntervals: {}".format(hyp))

#Result: Intervals: [(0.010681780036703725, 0.24697634733123064), (0.49696588960744587, 0.72688024561992848)]


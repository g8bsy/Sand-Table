import numpy as np

from os import listdir, makedirs, path
from os.path import isfile, join


pending_folder = "pending/"

def get_processed_folder(max_disp):
    processed_folder = "processed/tmp/"
    #processed_folder = "processed/max_disp=" + str(max_disp) + "/STEPS_PER_REV=" + str(const.STEPS_PER_REV) + "/MICROSTEP_SIZE=" + str(const.MICROSTEP_SIZE) +  "/LIN_RESOLUTION=" + const.LIN_RESOLUTION.replace("/", "_") + "/ROT_RESOLUTION=" + const.ROT_RESOLUTION.replace("/", "_") + "/"
    isExist = path.exists(processed_folder)
    if not isExist:
        # Create a new directory because it does not exist
        makedirs(processed_folder)    
    return processed_folder

def get_files(Dir="", folder=pending_folder):
    onlyfiles = [f for f in listdir(Dir + folder) if isfile(join(folder, f))]
    print("Tracks found: " + str(onlyfiles).replace('[', '').replace(']', ''))

    return onlyfiles


def get_coors(filename, folder, max_disp):
    with open(folder + filename, "r") as f:
        content = f.readlines()

    lines = [line.rstrip('\n') for line in content]

    coors = np.array([0, 0])
    for c in lines:
        print(c);
        c = c.split(" ")
        theta = float(c[0])
        print("theta=" + str(theta));
        r = float(c[1])
        print("r=" + str(r));

        if (theta != 0 or r != 0):
            theta = int(8 * 10 * theta / 6.28318531)
            print("theta_c=" + str(theta));
            #theta = int(const.MICROSTEP_SIZE * const.STEPS_PER_REV * theta / 6.28318531)
            r = int(max_disp * r)
            print("r_c=" + str(r));
            print();
            coors = np.vstack((coors, [theta, r]))
            
    
    min_value = coors[1:, 0].min()
    coors[1:, 0] -= min_value
    
    return coors


def get_steps(filename, folder, max_disp):
    coors = get_coors(filename, folder, max_disp)
    print (coors)
    cts = coors_to_steps(coors)
    print ()
    print (cts)
    return cts


def coors_to_steps(coors):
    return coors[1:] - coors[:-1]


def add_delays(steps):

    min_delay = 0.001

    delays = np.array([0, 0])
    for s in steps:
        if abs(s[0]) > abs(s[1]):
            Rot_delay = min_delay
            Lin_delay = round(abs(s[0]) * min_delay / abs(s[1]), 10) if s[1] != 0 else -1
        elif abs(s[1]) > abs(s[0]):
            Rot_delay = round(abs(s[1]) * min_delay / abs(s[0]), 10) if s[0] != 0 else -1
            Lin_delay = min_delay
        else:
            Rot_delay = min_delay
            Lin_delay = min_delay

        delays = np.vstack((delays, [Rot_delay, Lin_delay]))

    delays = delays[1:]
    steps_with_delays = np.concatenate((steps, delays), axis=1)

    return steps_with_delays


def process_new_files(Dir="", max_disp=2000):
    print("----- Pending files -----")
    pending_files = set(get_files(Dir=Dir, folder=pending_folder))
    print("----- Processed files -----")
    processed_files = set(get_files(Dir=Dir, folder=get_processed_folder(max_disp)))
    new_files = list(pending_files - processed_files)
    print("----- New files -----\nTracks found: " + str(new_files).replace('[', '').replace(']', ''))

    if len(new_files) > 0:
        tracks = {}

        for f in new_files:
            steps_with_delays = add_delays(get_steps(f, Dir + pending_folder, max_disp))
            tracks.update({f: steps_with_delays})

        write_tracks(tracks, Dir, max_disp)


def write_tracks(tracks, Dir="", max_disp=2000):
    for name in tracks:
        print("Writing " + Dir + get_processed_folder(max_disp) + name)
        current_file = open(Dir + get_processed_folder(max_disp) + name, "w+")
        for line in tracks[name]:
            current_file.write("{} {} {} {}\n".format(int(line[0]), int(line[1]), line[2], line[3]))
        current_file.close()

    print("\nFiles processed!\n")


def read_track(filename, Dir="", max_disp=2000):
    with open(Dir + get_processed_folder(max_disp) + filename, "r") as f:
        content = f.readlines()

    lines = [line.rstrip('\n') for line in content]

    coors = np.array([0, 0, 0, 0])
    for c in lines:
        step_coor = c.split(" ")
        step_coor[0] = int(float(step_coor[0]))
        step_coor[1] = int(float(step_coor[1]))
        step_coor[2] = float(step_coor[2])
        step_coor[3] = float(step_coor[3])

        coors = np.vstack((coors, step_coor))

    return coors[1:]


if __name__ == '__main__':
    process_new_files(Dir="../")
    print("\n{}".format(read_track("SwoopyRadiance.txt", Dir="../")))

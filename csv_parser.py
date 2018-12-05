import csv

path = r"G:\My Drive\Classes\4760\Final Project\hygdata_v3.csv"
ID, hip, hd, hr, gl = [], [], [], [], []
bf, proper, ra, dec, dist = [], [], [], [], []
pmra, pmdec, rv, mag, absmag = [], [], [], [], []
spect, ci, x, y, z = [], [], [], [], []
vx, vy, vz, rarad, decrad = [], [], [], [], []
pmrarad, pmdecrad, bayer, flam, con = [], [], [], [], []
comp, comp_primary, base, lum, var, var_min, var_max = [], [], [], [], [], [], []
con_full_name = {} # make dict from abbreviated names in con to full names
# call this to convert the csv into arrays grouped by columns
def setup_csv():
    with open(path) as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            if (row['proper'] != ''):
                ID.append(row['id'])
                hip.append(row['hip'])
                hd.append(row['hd'])
                hr.append(row['hr'])
                gl.append(row['gl'])
                bf.append(row['bf'])
                proper.append(row['proper'])
                ra.append(row['ra'])
                dec.append(row['dec'])
                dist.append(row['dist'])
                pmra.append(row['pmra'])
                pmdec.append(row['pmdec'])
                rv.append(row['rv'])
                mag.append(row['mag'])
                absmag.append(row['absmag'])
                spect.append(row['spect'])
                ci.append(row['ci'])
                x.append(row['x'])
                y.append(row['y'])
                z.append(row['z'])
                vx.append(row['vx'])
                vy.append(row['vy'])
                vz.append(row['vz'])
                rarad.append(row['rarad'])
                decrad.append(row['decrad'])
                pmrarad.append(row['pmrarad'])
                pmdecrad.append(row['pmdecrad'])
                bayer.append(row['bayer'])
                flam.append(row['flam'])
                con.append(row['con'])
                comp.append(row['comp'])
                comp_primary.append(row['comp_primary'])
                base.append(row['base'])
                lum.append(row['lum'])
                var.append(row['var'])
                var_min.append(row['var_min'])
                var_max.append(row['var_max'])
    for i in range (len(dec)):
        x_abs.append(sin(dec)*cos(ra*15))
        y_abs.append(sin(dec)*cos(ra*15))
        z_abs.append(cos(dec))

# search through arrays
def search_csv(ra_in, dec_in):
    x_temp.append(sin(dec_in)*cos(ra_in*15))
    y_temp.append(sin(dec_in)*cos(ra_in*15))
    z_temp.append(cos(dec_in))
    min_ = 2**31
    ind = -1
    for i in range(len(x_abs)):
        x = abs(x_abs-x_temp)
        y = abs(y_abs-y_temp)
        z = abs(z_abs-z_temp)
        if (x + y + z < min_):
            min_ = x + y + z
            ind = i
    if (ind == -1):
        return 'error, nothing found'
    else:
        return proper[ind]

setup_csv()
print ID
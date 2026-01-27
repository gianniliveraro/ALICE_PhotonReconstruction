import numpy as np
import pandas as pd
import plotly.graph_objects as go

# ==========================================
# Physical constants and settings
# ==========================================
Bz = 0.5          # Tesla
n_points = 100    # Number of points per track
s_max = 200       # cm
CollisionID = 1  # Select tracks from this collision only
# ==========================================
# Reading data
# ==========================================
df = pd.read_parquet("AO2D_1.parquet")
df = df[df.fIndexCollisions == CollisionID]

print("N. Charged tracks:", len(df))

# ==========================================
# Track propagation function
# ==========================================
def propagate_track(row):
    x0, y0, z0 = row["fX"], row["fY"], row["fZ"]
    alpha = row["fAlpha"]
    snp = row["fSnp"]
    tgl = row["fTgl"]
    fsigpT = row["fSigned1Pt"]
    fpT = 1.0 / abs(fsigpT)  # GeV
    sign = np.sign(fsigpT)

    R = (fpT / (0.3 * Bz)) * 100  # cm

    phi0 = np.arcsin(np.clip(snp, -1, 1))
    s = np.linspace(0, s_max, n_points)

    x_loc = x0 + R * (np.sin(phi0 + sign * s / R) - np.sin(phi0))
    y_loc = y0 - R * (np.cos(phi0 + sign * s / R) - np.cos(phi0))
    z = z0 + s * tgl

    # Rotation
    cos_a, sin_a = np.cos(alpha), np.sin(alpha)
    x = x_loc * cos_a - y_loc * sin_a
    y = x_loc * sin_a + y_loc * cos_a

    # N.B.: swapping axes for visualization (z → x, x → y, y → z)
    return z, x, y

# ==========================================
# Figure
# ==========================================
fig = go.Figure()

# ==========================================
# ITS + TPC layers
# ==========================================
DetectorsRadius_mm = [
    22.4, 30.1, 37.8, 194.4, 243.9, 342.3, 391.8,
    610, 788, 848, 2466, 2580, 2780
]

DetectorsZ_cm = [
    27, 27, 27, 84, 84, 147, 147, # ITS
    500, 500, 500, 500, 500, 500  # TPC
]

Detectors_Opacity = [
    0.4, 0.4, 0.4, 0.3, 0.3, 0.2, 0.2, # ITS
    0.15, 0.15, 0.15, 0.15, 0.15, 0.15  # TPC
]

DetectorsColors = [
    "#cb6682", "#cb6682", "#cb6682",
    "#78bf88", "#78bf88",
    "#6962b1", "#6962b1",
    "#476a71", "#476a71", "#476a71",
    "#476a71", "#476a71", "#4cbc70"
]

phi = np.linspace(0, 2*np.pi, 60)

Nsurfaces = 0
for r_mm, z_cm, color in zip(DetectorsRadius_mm, DetectorsZ_cm, DetectorsColors):
    r = r_mm * 0.1  # mm → cm

    z_extent = np.linspace(-z_cm/2, z_cm/2, 2)  # cm

    Phi, Z = np.meshgrid(phi, z_extent)

    Y = r * np.cos(Phi)
    Zc = r * np.sin(Phi)
    X = Z 

    fig.add_trace(go.Surface(
        x=X,
        y=Y,
        z=Zc,
        showscale=False,
        opacity=Detectors_Opacity[Nsurfaces],
        surfacecolor=np.ones_like(X),
        colorscale=[[0, color], [1, color]]
    ))

    Nsurfaces += 1
# ==========================================
# Tracks
# ==========================================
for _, row in df.iterrows():
    x, y, z = propagate_track(row)
    pT = 1.0 / abs(row["fSigned1Pt"])  # GeV
    charge = np.sign(row["fSigned1Pt"])

    line=dict(width=2, color="blue" if charge < 0 else "red")
    fig.add_trace(go.Scatter3d(
        x=x,
        y=y,
        z=z,
        mode="lines",
        line=line,
        opacity=0.8,

        # track metadata
        customdata=np.full(len(x), pT),

        hovertemplate=(
            "p<sub>T</sub>: %{customdata:.3f} GeV/c<br>"
            "charge: " + ("+" if charge > 0 else "-") + "<br>"
            "Z: %{x:.1f} cm<br>"
            "X: %{y:.1f} cm<br>"
            "Y: %{z:.1f} cm"
            "<extra></extra>"
        )
    ))


# ==========================================
# Layout
# ==========================================
fig.update_layout(
    title="ALICE Event Display – ITS + TPC + Tracks",
    scene=dict(
        xaxis_title="Z  [cm]",
        yaxis_title="X  [cm]",
        zaxis_title="Y  [cm]",
        aspectmode="data",
        #bgcolor="black"
    ),
    #paper_bgcolor="black",
    #font=dict(color="white"),
    showlegend=False
)

fig.show()


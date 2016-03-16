return {
    -- Mars barycenter module
    {
        Name = "MarsBarycenter",
        Parent = "SolarSystemBarycenter",
        Ephemeris = {
            Type = "Static"
        }
    },

    -- Mars module
    {   
        Name = "Mars",
        Parent = "MarsBarycenter",
        Renderable = {
            Type = "RenderablePlanet",
			Frame = "IAU_MARS",
			Body = "MARS",
            Geometry = {
                Type = "SimpleSphere",
                Radius = { 6.390, 6 },
                Segments = 100
            },
            Textures = {
                Type = "simple",
                Color = "textures/mars.jpg",
            },
            Atmosphere = {
                Type = "Nishita", -- for example, values missing etc etc
                MieFactor = 1.0,
                MieColor = {1.0, 1.0, 1.0}
            }
        },
        Ephemeris = {
            Type = "Spice",
            Body = "MARS",
            Reference = "ECLIPJ2000",
            Observer = "SUN",
            Kernels = {
                "${OPENSPACE_DATA}/spice/MAR063.BSP"
            }
        },
        Rotation = {
            Type = "Spice",
            Frame = "IAU_MARS",
            Reference = "ECLIPJ2000"
        },
        GuiName = "/Solar/Planets/Mars"
    },
    -- MarsTrail module
    {   
        Name = "MarsTrail",
        Parent = "MarsBarycenter",
        Renderable = {
            Type = "RenderableTrail",
            Body = "MARS",
            Frame = "GALACTIC",
            Observer = "SUN",
            RGB = { 1, 0.8, 0.5 },
            TropicalOrbitPeriod = 686.973,
            EarthOrbitRatio = 1.881,
            DayLength = 24.6597,
            Textures = {
                Type = "simple",
                Color = "${COMMON_MODULE}/textures/glare_blue.png",
                -- need to add different texture
            },  
        },
        GuiName = "/Solar/MarsTrail"
    }
}
#!/usr/bin/env python
"""
_AlCaPhiSymEcal_

Scenario supporting proton collisions

"""

from Configuration.DataProcessing.AlCa import AlCa

class AlCaPhiSymEcal(AlCa):
    def __init__(self):
        self.skims=['EcalCalPhiSym']
    """
    _AlCaPhiSymEcal_

    Implement configuration building for data processing for proton
    collision data taking

    """

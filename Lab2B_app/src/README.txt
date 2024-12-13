\documentclass[10pt, a4paper]{article}
\usepackage{geometry}
\usepackage{graphicx}
\usepackage{hyperref}
\usepackage{listings}
\geometry{margin=1in}

\title{XPT2046 Touch Controller Interface Manual}
\author{Michael Smith}
\date{December, 2024}

\begin{document}

\maketitle

\tableofcontents

\section{Introduction}
This document serves as a comprehensive reference manual for interfacing the XPT2046 touch controller with the Nexys A7 FPGA board. It includes detailed descriptions of hardware setup, software design, and implementation considerations based on the XPT2046 datasheet.

\section{Hardware Overview}
\subsection{XPT2046 Touch Controller}
The XPT2046 is a 4-wire resistive touch screen controller capable of providing precise X and Y touch coordinates over SPI communication. Key features include:
\begin{itemize}
    \item 12-bit resolution for X, Y, and pressure measurements.
    \item SPI communication with a maximum clock frequency of 2 MHz.
    \item Typical DCLK cycle time of 200 ns.
    \item Single supply voltage of 2.7V to 5.5V.
\end{itemize}

\textbf{Relevant Datasheet Information:}
\begin{itemize}
    \item \textbf{Commands:} \texttt{0x90} (X-axis), \texttt{0xD0} (Y-axis), \texttt{0xB0} (Z1), and \texttt{0xC0} (Z2) initiate ADC conversions for the respective coordinates.
    \item \textbf{Timing:} Ensure the SPI clock respects the 200 ns minimum DCLK period.
    \item \textbf{Power:} Operates at 3.3V, compatible with the Nexys A7's voltage levels.
\end{itemize}
\begin{center}
    \includegraphics[width=0.5\textwidth]{image1.png}
\end{center}
\subsection{Nexys A7 FPGA Board}
The Nexys A7, powered by an Artix-7 FPGA, provides the necessary GPIOs and AXI Quad SPI block for interfacing with the XPT2046. The FPGA's flexibility allows for precise timing control and efficient processing of touch data.

\section{Hardware Connections}
\subsection{XPT2046 Connections}
\begin{itemize}
    \item \textbf{T\_CLK or DCLK(Clock):} Connected to SPI sck\_o.
    \item \textbf{T\_CS or CS(Chip Select):} Connected to the SPI ss\_o[0:1] pins.
    \item \textbf{T\_DIN or DIN(MOSI):} Connected to SPI MOSI which is the io0\_o pin.
    \item \textbf{T\_DO or DO(MISO):} Connected to SPI MISO which is the io1\_i pin.
    \item \textbf{T\_IRQ or PENIRQ(Interrupt):} Connected to a GPIO for touch detection (active-low) which would be the gpio\_io\_i pin.
\end{itemize}
The figures below show a proper wiring schematic for the touch controller pins paired with an LCD display. Us one of the Jmods on the Nexys boards to connect these wires depending on your hardware constraints files.

\includegraphics[width=0.5\textwidth]{image2.png}
\includegraphics[width=0.5\textwidth]{image3.png}

\section{Software Implementation}
\subsection{Initialization}
The software initializes the AXI Quad SPI block for master mode with manual slave select. Key Initialization Steps:
\begin{itemize}
    \item Initialize the SPI:
    \begin{lstlisting}[language=C, caption=SPI Initializationl]
    XSpi_Config *spiConfig; /* Pointer to Configuration data */
    u32 status;
    
    spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);
    if (spiConfig == NULL) {
    xil_printf("Can't find spi device!\n");
    return XST_DEVICE_NOT_FOUND;
    }
    
    status = XSpi_CfgInitialize(&spi, spiConfig, spiConfig->BaseAddress);
    if (status != XST_SUCCESS) {
    xil_printf("Initialize spi fail!\n");
    return XST_FAILURE;
    }

\end{lstlisting}
    \item Configure the SPI control register to enable master mode and slave select using either XSpi\_SetOption() or XSpi\_getControlReg():
    \begin{lstlisting}[language=C, caption=SPI Initializationl]
    XSpi_SetOptions(&spi, XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION);
\end{lstlisting}
    \begin{lstlisting}[language=C, caption=SPI Initializationl]
    controlReg = XSpi_GetControlReg(&spi);
    XSpi_SetControlReg(&spi,
    (controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) &
    (~XSP_CR_TRANS_INHIBIT_MASK));
\end{lstlisting}
    \item Start the SPI and disable any global interrupts that may interfere with the communication bus:
    \begin{lstlisting}[language=C, caption=SPI Initializationl]
    XSpi_Start(&spi);
    XSpi_IntrGlobalDisable(&spi);
\end{lstlisting}
\end{itemize}
With these steps, the SPI driver should be properly configured to handle the touch controller communication. If not, SPI library functions like XSpi\_GetOptions(), XSpi\_GetControlReg(), and XSpi\_GetStatusReg() can be used to check the driver status. Detecting touch is possible through polling either the Z pressure data or the T\_IRQ/PENIRQ interrupt. If using the interrupt, make sure the PENIRQ doesn't effect the clock or otherwise it will disrupt the transfer.

\subsection{XPT2046 Touch Controller Commands}
The following commands are used to retrieve touch coordinates:
\begin{itemize}
    \item \textbf{\texttt{0x90}}: Reads the X-coordinate.
    \item \textbf{\texttt{0xD0}}: Reads the Y-coordinate.
    \item \textbf{\texttt{0xB0}}: Reads the Z1-pressure.
    \item \textbf{\texttt{0xC0}}: Reads the Z2-pressure.
\end{itemize}
A command represents a specific control byte for the touch controller to read from. each control byte initiates an ADC conversion. The results are retrieved as 12-bit values, which requires two SPI transfers. The entire transaction is performed using XSpi\_Transfer(), passing both the command and an empty buffer for the received data. Make sure the 12 bits returned are converted corrected.

\subsection{Code Highlights}
Below is an example function that implements the transfer. The command parameter represents the control byte mentioned earlier to retrieve specific values from the touch:
\begin{lstlisting}[language=C, caption=Touch Coordinate Retrieval]
uint16_t TouchTransfer(uint8_t command) {
    uint8_t txBuffer[3] = {command, 0x00, 0x00};
    uint8_t rxBuffer[3] = {0x00, 0x00, 0x00};
    uint16_t coordinate;

    XSpi_SetSlaveSelect(&SpiInstance, Touch_Slave_Pin); // Select the touch
    XSpi_Transfer(&SpiInstance, txBuffer, rxBuffer, 3);
    XSpi_SetSlaveSelect(&SpiInstance, 0x00); // Deselect the touch

    coordinate = ((rxBuffer[0] << 8) | rxBuffer[1]) >> 4;
    return coordinate;
}
\end{lstlisting}
The function utilizes the XSpi\_SetSlaveSelect() function to select and deselect the touch slave. If the SPI is setup to handle multiple slaves, it is important to properly select and deselect when talking to different slaves. 

\subsection{Interrupt and Polling}
Polling of T\_IRQ ensures efficient use of resources. If interrupts are desired, the GPIO interrupt handler must be configured to detect low signals on T\_IRQ. Without interrupts, touches can be detected using a polling method while checking Z1 and Z2 pressure data. The below function returns true if a press has occurred based on the Z1 and Z2 data received after a transfer. Polling this inside a while loop will allow you to request and receive X and Y coordinates when a touch has occurred.
\begin{lstlisting}[language=C, caption=Touch Coordinate Retrieval]
int is_touched(uint16_t *z1, uint16_t *z2) {
    *z1 = TouchTransfer(0xB0);
    *z2 = TouchTransfer(0xC0);

    // Simple pressure check
    return (*z1 > 10 && *z2 < 2000);
}
\end{lstlisting}

\section{Test and Validation}
\subsection{Loopback Tests}
To verify SPI communication, a loopback test was performed by shorting MISO and MOSI. The successful data echo confirmed the SPI setup. Replicating this involving directly connecting the MISO and MOSI pins and observing the bytes sent and received. If the MOSI and MISO of the SPI is set up correctly, the data sent will be the same as the data received.

\subsection{Logic Analyzer Outputs}
When testing with a Logic Analyzer, the following should be observed over the lines when data is being successfully transferred.

\begin{center}
    \includegraphics[width=0.5\textwidth]{image4.png}
\end{center}
\begin{itemize}
    \item \textbf{MOSI or DIN:} Command bytes (e.g., \texttt{0x90} and \texttt{0xD0}).
    \item \textbf{MISO or DO:} 12-bit ADC results for X and Y coordinates.
    \item \textbf{SCK or DCLK:} Stable clock signal matching the configured frequency of 2 MHz.
\end{itemize}

\section{Conclusion}
The integration of the XPT2046 touch controller with the Nexys A7 FPGA was successfully achieved. Challenges such as SPI timing mismatches and touch inconsistencies were resolved through careful debugging and adherence to datasheet specifications. Future improvements include interrupt-driven touch detection and multiple touch recognition.

\end{document}

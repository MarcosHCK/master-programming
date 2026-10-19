/*
  PERCEPTRÓN DIABETES - ARDUINO UNO / WOKWI
  Entrada serie: índice de masa corporal y glucosa
  0 = sin diabetes; 1 = con diabetes (clasificación académica)
*/

// PESOS REPORTADOS POR clase011/diabetes.csv (bias primero):
const float W0 = 0.18529315;
const float W1 = 2.9604141;
const float W2 = 2.4754073;

const float MEDIA_IMC = 27.038039;
const float MEDIA_GLUCOSE = 123.17412;

const float ESCALA_IMC = 3.5460616;
const float ESCALA_GLUCOSE = 31.741446;

bool numeroValido (String texto)
{

  texto.trim ();
  if (! texto.length ()) return false;

  bool digito = false;
  bool punto = false;

  for (unsigned int i = 0; i < texto.length (); i++)
    {

      char caracter = texto.charAt (i);

      if (caracter >= '0' && caracter <= '9')
        {
          digito = true;
          continue;
        }

      if (caracter == '.' && ! punto)
        {
          punto = true;
          continue;
        }

      return false;
    }

  return digito;
}

void setup ()
{

  Serial.begin (9600);
  Serial.setTimeout (1000);
  Serial.println ("PERCEPTRÓN DIABETES - 2 ENTRADAS");
  Serial.println ("Introduzca el IMC y la glucosa separados por comas (ejemplo: 30.5,155).");
  Serial.println ("Pesos entrenados en clase011/diabetes.csv:");
  Serial.print ("W0 = "); Serial.println (W0, 6);
  Serial.print ("W1 = "); Serial.println (W1, 6);
  Serial.print ("W2 = "); Serial.println (W2, 6);
}

void loop ()
{

  if (! Serial.available ()) return;

  String linea = Serial.readStringUntil ('\n');
  linea.trim ();

  if (! linea.length ()) return;

  int coma = linea.indexOf (',');

  if (coma < 0 || linea.indexOf (',', coma + 1) >= 0)
    {
      Serial.println ("ERROR. Formato: IMC,Glucosa");
      return;
    }

  String campoIMC = linea.substring (0, coma);
  String campoGlucosa = linea.substring (coma + 1);

  if (! numeroValido (campoIMC) || ! numeroValido (campoGlucosa))
    {
      Serial.println ("ERROR. Utilice números positivos");
      return;
    }

  float imc = campoIMC.toFloat ();
  float glucosa = campoGlucosa.toFloat ();

  if (imc <= 0 || glucosa <= 0)
    {
      Serial.println ("ERROR. Ambos valores deben ser positivos");
      return;
    }

  float Z = W0
            + W1 * ((imc - MEDIA_IMC) / ESCALA_IMC)
            + W2 * ((glucosa - MEDIA_GLUCOSE) / ESCALA_GLUCOSE);
  int Y = Z >= 0 ? 1 : 0;

  Serial.print ("Z = "); Serial.println (Z, 6);
  Serial.print ("Y = "); Serial.println (Y);
  Serial.println (Y ? "Clase: CON DIABETES (1)" : "Clase: SIN DIABETES (0)");
  Serial.println ("Ingrese otro registro: IMC,Glucosa");
}
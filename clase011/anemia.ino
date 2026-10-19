/*
  PERCEPTRON ANEMIA - ARDUINO UNO / WOKWI
  Entrada serie: Edad,Energia,Frecuencia,Antecedentes
  0 = sin anemia; 1 = con anemia (clasificacion academica)
*/

// PESOS REPORTADOS POR clase011/anemia.csv (bias primero):
const float W0 = -0.73065623;
const float W1 = 0.28345225;
const float W2 = -1.5932475;
const float W3 = 1.5216476;
const float W4 = 3.4369973;

const float MEDIA_EDAD = 35.051813;
const float MEDIA_ENERGIA = 5.7772021;
const float MEDIA_FRECUENCIA = 86.321244;
const float MEDIA_ANTECEDENTES = 0.44041451;

const float ESCALA_EDAD = 11.968759;
const float ESCALA_ENERGIA = 1.6624778;
const float ESCALA_FRECUENCIA = 9.202974;
const float ESCALA_ANTECEDENTES = 0.49643687;

void setup ()
{

  Serial.begin (9600);
  Serial.setTimeout (1000);
  Serial.println ("PERCEPTRON ANEMIA - 4 ENTRADAS");
  Serial.println ("Introduzca: Edad,Energia,Frecuencia,Antecedentes");
  Serial.println ("Pesos entrenados en clase011/anemia.csv:");
  Serial.print ("W0 = "); Serial.println (W0, 6);
  Serial.print ("W1 = "); Serial.println (W1, 6);
  Serial.print ("W2 = "); Serial.println (W2, 6);
  Serial.print ("W3 = "); Serial.println (W3, 6);
  Serial.print ("W4 = "); Serial.println (W4, 6);
}

void loop ()
{

  if (! Serial.available ()) return;

  String linea = Serial.readStringUntil ('\n');
  linea.trim ();

  int coma1 = linea.indexOf (',');
  int coma2 = linea.indexOf (',', coma1 + 1);
  int coma3 = linea.indexOf (',', coma2 + 1);

  if (coma1 < 0 || coma2 < 0 || coma3 < 0 || linea.indexOf (',', coma3 + 1) >= 0)
    {
      Serial.println ("ERROR. Formato: Edad,Energia,Frecuencia,Antecedentes");
      return;
    }

  String campo1 = linea.substring (0, coma1);
  String campo2 = linea.substring (coma1 + 1, coma2);
  String campo3 = linea.substring (coma2 + 1, coma3);
  String campo4 = linea.substring (coma3 + 1);

  campo1.trim ();
  campo2.trim ();
  campo3.trim ();
  campo4.trim ();

  if (! campo1.length () || ! campo2.length () || ! campo3.length () ||
      (campo4 != "0" && campo4 != "1"))
    {
      Serial.println ("ERROR. Datos incompletos o antecedentes invalidos");
      return;
    }

  float edad = campo1.toFloat ();
  float energia = campo2.toFloat ();
  float frecuencia = campo3.toFloat ();
  int antecedentes = campo4.toInt ();

  if (edad <= 0 || energia < 0 || frecuencia <= 0)
    {
      Serial.println ("ERROR. Valores fuera de rango");
      return;
    }

  float Z = W0
            + W1 * ((edad - MEDIA_EDAD) / ESCALA_EDAD)
            + W2 * ((energia - MEDIA_ENERGIA) / ESCALA_ENERGIA)
            + W3 * ((frecuencia - MEDIA_FRECUENCIA) / ESCALA_FRECUENCIA)
            + W4 * ((antecedentes - MEDIA_ANTECEDENTES) / ESCALA_ANTECEDENTES);
  int Y = Z >= 0 ? 1 : 0;

  Serial.print ("Z = "); Serial.println (Z, 6);
  Serial.print ("Y = "); Serial.println (Y);
  Serial.println (Y ? "Clase: CON ANEMIA (1)" : "Clase: SIN ANEMIA (0)");
  Serial.println ("Uso academico; no es diagnostico medico.");
  Serial.println ("Ingrese otro registro:");
}